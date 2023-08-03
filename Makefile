lib.name = ekext
#
class.sources = bmt~.c copyarray.c cup.c cupd.c doubledelta.c floatcount.c floatcup.c framescore~.c framespect~.c groupsplit.c hasc~.c hssc~.c ihisto.c isoWrap~.c lcmgcd.c list_sum.c markovChains.c maskxor.c mvcf~.c ninjacount.c ninjalist.c phasorbars~.c polymap.c polystat.c positive.c ptwo.c regroup.c rootint.c scalefilter.c sieve.c simile~.c simile.c stavelines.c steady.c valve.c voicing_detector~.c wavecutter~.c wavefolder~.c wavestretcher~.c weightonset.c wfold~.c wrap_overshoot~.c zeroxpos~.c
#
datafiles = bmt~-help.pd count-help.pd count.pd cupd-help.pd cup-help.pd doubledelta-help.pd ekext-meta.pd floatcount-help.pd framescore~-help.pd framespect~-help.pd hasc~-help.pd hssc~-help.pd ihisto-help.pd isoWrap~-help.pd lcmgcd-help.pd list_sum-help.pd markovChains-help.pd maskxor-help.pd maskxor-test.pd mvcf~-help.pd polymap-help.pd polystat-help.pd positive-help.pd scalefilter-help.pd sieve-help.pd simile~-help.pd simile-help.pd steady-help.pd test_scalefilter.pd valve-help.pd voicing_detector~-help.pd wavecutter~-help.pd wavefolder~-help.pd wavestretcher~-help.pd wfold~-help.pd weightonset-help.pd zeroxpos~-help.pd README.txt LICENSE.txt
#

#PDLIBBUIDER_DIR=./pd-lib-builder
#include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder
include Makefile.pdlibbuilder
