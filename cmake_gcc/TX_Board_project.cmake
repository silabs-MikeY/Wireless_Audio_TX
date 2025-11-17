#eyJzdGF0ZSI6eyJidWlsZFByZXNldHMiOnsiZGVmYXVsdF9jb25maWciOnsibmFtZSI6ImRlZmF1bHRfY29uZmlnIiwiY29uZmlndXJhdGlvbiI6ImJhc2UiLCJkZWZpbml0aW9ucyI6W10sInJlbW92ZWRGbGFncyI6eyJDIjpbIi1PZyJdLCJDWFgiOlsiLU9nIl0sIkFTTSI6W119LCJhZGRpdGlvbmFsRmxhZ3MiOnsiQyI6WyItTzAiXSwiQ1hYIjpbIi1PMCJdLCJBU00iOltdfX19LCJjdXJyZW50Q29uZkluZGV4IjowfSwiZm9sZGVycyI6WyJyYWRpbyJdLCJmaWxlcyI6W119
include_directories(
	"../radio"
)

target_sources(TX_Board PRIVATE
	"../ADC.c"
	"../RGB.c"
	"../VDAC.c"
	"../audio_buffer.c"
	"../button.c"
	"../debug.c"
	"../events_print.c"
	"../generic.c"
	"../microseconds.c"
	"../radio.c"
	"../scheduler.c"
	"../state_machine.c"
	"../timer_helper.c"
	"../wdog.c"
	"../radio/radio_base.c"
	"../radio/radio_receive.c"
	"../radio/radio_retry.c"
	"../radio/radio_statistics.c"
	"../radio/radio_transmit.c"
)

get_target_property(interface_compile_options slc INTERFACE_COMPILE_OPTIONS)
	list(REMOVE_ITEM interface_compile_options $<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:C>>:-Og>)
	list(REMOVE_ITEM interface_compile_options $<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:CXX>>:-Og>)
set_target_properties(TX_Board PROPERTIES INTERFACE_COMPILE_OPTIONS "${interface_compile_opitions}")

target_compile_options(TX_Board PRIVATE
	$<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:C>>:-O0>
	$<$<AND:$<CONFIG:default_config>,$<COMPILE_LANGUAGE:CXX>>:-O0>
)
