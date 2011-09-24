/*
  landslide.c - A Module for Simics which provides yon Hax and Sploits

  Copyright 1998-2009 Virtutech AB
  
  The contents herein are Source Code which are a subset of Licensed
  Software pursuant to the terms of the Virtutech Simics Software
  License Agreement (the "Agreement"), and are being distributed under
  the Agreement.  You should have received a copy of the Agreement with
  this Licensed Software; if not, please contact Virtutech for a copy
  of the Agreement prior to using this Licensed Software.
  
  By using this Source Code, you agree to be bound by all of the terms
  of the Agreement, and use of this Source Code is subject to the terms
  the Agreement.
  
  This Source Code and any derivatives thereof are provided on an "as
  is" basis.  Virtutech makes no warranties with respect to the Source
  Code or any derivatives thereof and disclaims all implied warranties,
  including, without limitation, warranties of merchantability and
  fitness for a particular purpose and non-infringement.

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>
#include <simics/arch/x86.h>

// XXX: idiots wrote this header, so it must be after the other includes.
#include "trace.h"

#define MODULE_NAME "LANDSLIDE"
#define MODULE_COLOUR COLOUR_RED

#include "common.h"
#include "landslide.h"
#include "x86.h"

/******************************************************************************
 * simics glue
 ******************************************************************************/

static conf_object_t *ls_new_instance(parse_object_t *parse_obj)
{
	struct ls_state *ls = MM_ZALLOC(1, struct ls_state);
	assert(ls && "failed to allocate ls state");
	SIM_log_constructor(&ls->log, parse_obj);
	ls->trigger_count = 0;
	ls->absolute_trigger_count = 0;

	ls->cpu0 = SIM_get_object("cpu0");
	assert(ls->cpu0 && "failed to find cpu");
	ls->kbd0 = SIM_get_object("kbd0");
	assert(ls->kbd0 && "failed to find keyboard");

	sched_init(&ls->sched);
	arbiter_init(&ls->arbiter);
	save_init(&ls->save);
	test_init(&ls->test);

	return &ls->log.obj;
}

/* type should be one of "integer", "boolean", "object", ... */
#define LS_ATTR_SET_GET_FNS(name, type)				\
	static set_error_t set_ls_##name##_attribute(			\
		void *arg, conf_object_t *obj, attr_value_t *val,	\
		attr_value_t *idx)					\
	{								\
		((struct ls_state *)obj)->name = SIM_attr_##type(*val);	\
		return Sim_Set_Ok;					\
	}								\
	static attr_value_t get_ls_##name##_attribute(			\
		void *arg, conf_object_t *obj, attr_value_t *idx)	\
	{								\
		return SIM_make_attr_##type(				\
			((struct ls_state *)obj)->name);		\
	}

/* type should be one of "\"i\"", "\"b\"", "\"o\"", ... */
#define LS_ATTR_REGISTER(class, name, type, desc)			\
	SIM_register_typed_attribute(class, #name,			\
				     get_ls_##name##_attribute, NULL,	\
				     set_ls_##name##_attribute, NULL,	\
				     Sim_Attr_Optional, type, NULL,	\
				     desc);

LS_ATTR_SET_GET_FNS(trigger_count, integer);
LS_ATTR_SET_GET_FNS(absolute_trigger_count, integer);

// XXX: figure out how to use simics list/string attributes
static set_error_t set_ls_arbiter_choice_attribute(
	void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	arbiter_append_choice(&((struct ls_state *)obj)->arbiter,
			      SIM_attr_integer(*val));
	return Sim_Set_Ok;
}
static attr_value_t get_ls_arbiter_choice_attribute(
	void *arg, conf_object_t *obj, attr_value_t *idx)
{
	return SIM_make_attr_integer(-42);
}

static set_error_t set_ls_save_path_attribute(
	void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	if (save_set_base_dir(&((struct ls_state *)obj)->save,
			      SIM_attr_string(*val))) {
		return Sim_Set_Ok;
	} else {
		return Sim_Set_Not_Writable;
	}
}
static attr_value_t get_ls_save_path_attribute(
	void *arg, conf_object_t *obj, attr_value_t *idx)
{
	const char *path = save_get_path(&((struct ls_state *)obj)->save);
	return SIM_make_attr_string(path);
}

static set_error_t set_ls_test_case_attribute(
	void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	if (cause_test(&((struct ls_state *)obj)->test,
		       ((struct ls_state *)obj)->kbd0,
		       SIM_attr_string(*val))) {
		return Sim_Set_Ok;
	} else {
		return Sim_Set_Not_Writable;
	}
}
static attr_value_t get_ls_test_case_attribute(
	void *arg, conf_object_t *obj, attr_value_t *idx)
{
	const char *path = test_get_test(&((struct ls_state *)obj)->test);
	return SIM_make_attr_string(path);
}


/* Forward declaration. */
static void ls_consume(conf_object_t *obj, trace_entry_t *entry);

/* init_local() is called once when the device module is loaded into Simics */
void init_local(void)
{
	const class_data_t funcs = {
		.new_instance = ls_new_instance,
		.class_desc = "hax and sploits",
		.description = "here we have a simix module which provides not"
			" only hax or sploits individually but rather a great"
			" conjunction of the two."
	};

	/* Register the empty device class. */
	conf_class_t *conf_class = SIM_register_class(SIM_MODULE_NAME, &funcs);

	/* Register the landslide class as a trace consumer. */
	static const trace_consume_interface_t sploits = {
		.consume = ls_consume
	};
	SIM_register_interface(conf_class, TRACE_CONSUME_INTERFACE, &sploits);

	/* Register attributes for the class. */
	LS_ATTR_REGISTER(conf_class, trigger_count, "i", "Count of haxes");
	LS_ATTR_REGISTER(conf_class, absolute_trigger_count, "i",
			 "Count of all haxes ever");
	LS_ATTR_REGISTER(conf_class, arbiter_choice, "i",
			 "Tell the arbiter which thread to choose next "
			 "(buffered, FIFO)");
	LS_ATTR_REGISTER(conf_class, save_path, "s",
			 "Base directory of saved choices for this test case");
	LS_ATTR_REGISTER(conf_class, test_case, "s",
			 "Which test case should we run?");

	lsprintf("welcome to landslide.\n");
}

/******************************************************************************
 * actual interesting landslide logic
 ******************************************************************************/

/* Main entry point. Called every instruction, data access, and extensible. */
static void ls_consume(conf_object_t *obj, trace_entry_t *entry)
{
	struct ls_state *ls = (struct ls_state *)obj;

	if (entry->trace_type != TR_Instruction)
		return;

	ls->trigger_count++;
	ls->absolute_trigger_count++;

	/* TODO: avoid using get_cpu_attr */
	ls->eip = GET_CPU_ATTR(ls->cpu0, eip);

	if (ls->trigger_count % 1000000 == 0) {
		lsprintf("hax number %d (%d) with trace-type %s at 0x%x\n",
			 ls->trigger_count, ls->absolute_trigger_count,
			 entry->trace_type == TR_Data ? "DATA" :
			 entry->trace_type == TR_Instruction ? "INSTR" : "EXN",
			 ls->eip);
	}

	if (ls->eip >= USER_MEM_START) {
		return;
	}

	// TODO: conditions for calling this?
	sched_update(ls);

	/* When a test case finishes, break the simulation so the wrapper can
	 * decide what to do. */
	if (test_update_state(&ls->test, &ls->sched) &&
	    !test_is_running(&ls->test)) {
		lsprintf("test case ended!\n");
		SIM_break_simulation(NULL);
	}
}
