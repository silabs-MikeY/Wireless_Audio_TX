#eyJzdGF0ZSI6eyJidWlsZFByZXNldHMiOnsiZGVmYXVsdF9jb25maWciOnsibmFtZSI6ImRlZmF1bHRfY29uZmlnIiwiY29uZmlndXJhdGlvbiI6ImJhc2UiLCJkZWZpbml0aW9ucyI6W10sInJlbW92ZWRGbGFncyI6eyJDIjpbIi1PZyJdLCJDWFgiOlsiLU9nIl0sIkFTTSI6W119LCJhZGRpdGlvbmFsRmxhZ3MiOnsiQyI6WyItTzAiXSwiQ1hYIjpbIi1PMCJdLCJBU00iOltdfX19LCJjdXJyZW50Q29uZkluZGV4IjowfSwiZm9sZGVycyI6WyJyYWRpbyJdLCJmaWxlcyI6W119
include_directories(
	"../libraries"
	"../radio"
)

target_sources(TX_Board PRIVATE
	"../libraries/ADC.c"
	"../audio_ring_buffer.c"
	"../libraries/RGB.c"
	"../libraries/VDAC.c"
	"../libraries/audio_buffers.c"
	"../libraries/button.c"
	"../libraries/counters_new.c"
	"../events_print.c"
	"../libraries/generic.c"
	"../libraries/microseconds.c"
	"../libraries/print.c"
	"../print_interfacing.c"
	"../radio.c"
	"../libraries/scheduler.c"
	"../state_machine.c"
	"../timer_helper.c"
	"../wdog.c"
	"../radio/radio_base.c"
	"../radio/radio_receive.c"
	"../radio/radio_retry.c"
	"../radio/radio_statistics.c"
	"../radio/radio_transmit.c"
	"../counter_interface.c"
	"../audio_pipeline.c"
	"../audio_encoding.c"
	"../adpcm.c"
)

get_target_property(interface_compile_options slc INTERFACE_COMPILE_OPTIONS)
	list(REMOVE_ITEM interface_compile_options $<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:C>>:-Og>)
	list(REMOVE_ITEM interface_compile_options $<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:CXX>>:-Og>)
set_target_properties(TX_Board PROPERTIES INTERFACE_COMPILE_OPTIONS "${interface_compile_opitions}")

target_compile_options(TX_Board PRIVATE
	$<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:C>>:-O0>
	$<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:CXX>>:-O0>
)
