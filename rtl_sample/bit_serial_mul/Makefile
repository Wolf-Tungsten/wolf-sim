SIM_OPTIONS := -full64 -sverilog -debug_access+all +v2k -kdb -lca -debug_region+cell+encrypt -fsdb +lint=TFIPC-L  -timescale=100ps/1ps
# -ntb_opts uvm-1.1

FILE_LIST_PATH := ${PWD}/bit_serial_mul_tb.f
INC_DIR_PATH   := ${PWD}

TOP_NAME  := bit_serial_mul_tb
WAVE_NAME := bit_serial_mul_tb

vcs_comp: clean
	vcs -f ${FILE_LIST_PATH} -top ${TOP_NAME} ${SIM_OPTIONS} -l vcs_comp.log +incdir+${INC_DIR_PATH}

vcs_sim: vcs_comp
	./simv -l vcs_sim.log

vcs_verdi:
	verdi -ssf ${WAVE_NAME}.fsdb

clean:
	rm -rf *.log *.daidir csrc simv* *.fsdb *.vpd *.vdb *.key novas.* *.
	clear