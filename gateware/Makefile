all:
	. venv/bin/activate && python3 deca_usb2_adc.py --keep-files

bench_adc:
	. venv/bin/activate && python3 adc-bench.py && gtkwave adc.vcd

bench:
	. venv/bin/activate && python3 adc-inject-data-bench.py && gtkwave adc-inject-data.vcd
