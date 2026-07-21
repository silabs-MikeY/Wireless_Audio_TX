#eyJzdGF0ZSI6eyJidWlsZFByZXNldHMiOnsiZGVmYXVsdF9jb25maWciOnsibmFtZSI6ImRlZmF1bHRfY29uZmlnIiwiY29uZmlndXJhdGlvbiI6ImJhc2UiLCJhZGRpdGlvbmFsRGVmaW5pdGlvbnMiOltdLCJyZW1vdmVkRGVmaW5pdGlvbnMiOltdLCJyZW1vdmVkRmxhZ3MiOnsiQyI6WyItT2ciXSwiQ1hYIjpbIi1PZyJdLCJBU00iOltdfSwiYWRkaXRpb25hbEZsYWdzIjp7IkMiOlsiLU8wIl0sIkNYWCI6WyItTzAiXSwiQVNNIjpbXX19fSwiY3VycmVudENvbmZJbmRleCI6MH0sImZvbGRlcnMiOlsicmFkaW8iXSwiZmlsZXMiOltdfQ==
include_directories(
	"../radio"
)

target_sources(TX_Board PRIVATE
	"../audio_encoding.c"
	"../audio_intensity.c"
	"../audio_pipeline.c"
	"../audio_ring_buffer.c"
	"../counter_interface.c"
	"../print_interfacing.c"
	"../radio_packet_buffers.c"
	"../libraries/state_machine.c"
	"../state_machine_interface.c"
	"../libraries/timer_helper.c"
	"../uart_sample_debug.c"
	"../ui_interfacing.c"
	"../radio/radio_base.c"
	"../radio/radio_receive.c"
	"../radio/radio_retry.c"
	"../radio/radio_statistics.c"
	"../radio/radio_transmit.c"
	"../radio/tx_retry.c"
)

target_compile_options(slc PUBLIC
	"$<$<AND:$<CONFIG:base>,$<COMPILE_LANGUAGE:C>>:-O0>"
	"$<$<AND:$<CONFIG:base>,$<COMPILE_LANGUAGE:CXX>>:-O0>"
)
