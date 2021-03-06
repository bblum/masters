
# -*- makefile -*-
# Do not edit this file.
# This file will be overwritten by the workspace setup script.

SIMICS_EXTENSION_BUILDER=/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-extension-builder-4.6.12
SIMICS_MODEL_BUILDER=/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-model-builder-4.6.28
SIMICS_BASE=/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58
SIMICS_WORKSPACE:=$(shell pwd)
PYTHON=$(SIMICS_WORKSPACE)/bin/mini-python

INCLUDE_PATHS = /afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/src/include:/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-extension-builder-4.6.12/src/include:/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-model-builder-4.6.28/src/include
CXX_INCLUDE_PATHS = /afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-extension-builder-4.6.12/linux64/api:/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-model-builder-4.6.28/linux64/api
DML_INCLUDE_PATHS = /afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-extension-builder-4.6.12/linux64/bin/dml/api:/afs/cs/academic/class/15410-s17/simics-4.6.58/simics-4.6.58/../simics-model-builder-4.6.28/linux64/bin/dml/api

# Put user definitions in config-user.mk
-include config-user.mk

# allow user to override HOST_TYPE
ifeq (,$(HOST_TYPE))
 HOST_TYPE:=$(shell $(PYTHON) $(SIMICS_BASE)/scripts/host_type.py)
endif

include compiler.mk

include $(SIMICS_MODEL_BUILDER)/config/workspace/config.mk

