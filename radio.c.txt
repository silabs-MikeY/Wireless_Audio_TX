// #include "radio.h"

// static bool radio__send_packet(uint32_t index_to_send);
// static uint32_t radio__create_new_packet_buffer(void);
// static bool radio__check_if_any_packet_ready_to_send(uint32_t *index);
// static bool radio__send_packet(uint32_t index_to_send);
// static void radio__try_to_send_a_packet(void);
// static bool radio__process_event_rx(RAIL_Handle_t rail_handle, RAIL_Events_t events);
// static bool radio__process_event_tx(RAIL_Handle_t rail_handle, RAIL_Events_t events);
// static void radio_process_receiver_packet(RAIL_Handle_t rail_handle);
// static void radio__error_handle(void);
// static bool radio__note_successful_tx(void);
// static bool radio__validate_radio_tx_count(void);
// static void radio__reset_radio_statistics(void);
// static bool radio__note_retry_sent(void);
// static bool radio__validate_radio_tx_timestamp_deltas(void);
// static uint32_t radio__find_packet_buffer_index_by_sequence_number(uint16_t sequence_number);
// static bool radio__send_packet_new(uint32_t buffer_index, bool retry);
// static void radio__mark_packet_as_sent(void);

// packet_buffer_t radio_packet_buffer[NUMBER_OF_PACKET_BUFFERS];
// volatile uint32_t next_sequence_number = 0;
// uint32_t active_buffer_index_left = 0;
// uint32_t active_buffer_index_right = 0;

// uint32_t left_data_received_count = 0;
// uint32_t right_data_received_count = 0;

// uint32_t channel = 10;
// bool channel_changed_flag = false;
// bool searching_for_channel_flag = false;

// bool sending_packet;

// uint16_t RX_FIFO_SIZE = RADIO_FIFO_SIZE;
// volatile uint8_t RX_FIFO[RADIO_FIFO_SIZE] __attribute__((aligned(4)));
// uint8_t tx_buffer[RADIO_FIFO_SIZE] __attribute__((aligned(4)));
// uint32_t FIFO_Size = 0;

// uint32_t packet_buffer_head = 0;
// uint32_t packet_buffer_tail = 0;

// volatile uint32_t number_of_TX_packets_sent_this_second = 0;
// volatile uint32_t last_tx_timestamp_micros = 0;
// volatile uint32_t max_tx_timestamp_delta = 0;
// volatile uint32_t min_tx_timestamp_delta = 0;

// mono_stereo_radio_config_values_t *stereo_or_mono_config = &mono_config;

// typedef struct bad_packet_timestamp_s
// {
//   uint32_t tx_timestamp;
//   uint32_t timestamp_delta_from_previous;
//   uint16_t sequence_number;
// } bad_packet_timestamp_t;
// #define BAD_TIMESTAMPS_SIZE 100
// bad_packet_timestamp_t bad_timestamps[BAD_TIMESTAMPS_SIZE];
// uint32_t bad_timestamps_count = 0;
// uint32_t moving_average_tx_delta = 0;

// typedef struct tx_packet_in_flight_info_s
// {
//   uint32_t packet_buffer_index;
//   bool retry;
//   bool in_flight;
// } tx_packet_in_flight_info_t;
// tx_packet_in_flight_info_t tx_packet_in_flight_info;



// uint32_t radio__get_sequence_number(void)
// {
//   return next_sequence_number;
// }

// static bool radio__validate_radio_tx_count(void)
// {

//   if ((number_of_TX_packets_sent_this_second >= ((stereo_or_mono_config->number_of_tx_per_second_expected) - 1)) && (number_of_TX_packets_sent_this_second <= ((stereo_or_mono_config->number_of_tx_per_second_expected) + 1)))
//   {
//     return true;
//   }
//   else
//   {
//     // MAYBE DO MORE
//     return false;
//   }
// }

// bool radio__validate_radio_tx_timestamp_deltas(void)
// {
//   bool too_slow_flag = false;
//   bool too_fast_flag = false;

//   if (min_tx_timestamp_delta < (stereo_or_mono_config->acceptable_tx_microsecond_delta_lower))
//   {
//     // TX ING TOO FAST
//     too_fast_flag = true;
//   }
//   if (max_tx_timestamp_delta > (stereo_or_mono_config->acceptable_tx_microsecond_delta_upper))
//   {
//     // TX ING TOO SLOW
//     too_slow_flag = true;
//   }
//   return (too_fast_flag | too_slow_flag);
// }

// void radio__get_tx_statistics(uint32_t *tx_timestamp_delta_max, uint32_t *tx_timestamp_delta_min, uint32_t *tx_count)
// {
//   tx_timestamp_delta_max = max_tx_timestamp_delta;
//   tx_timestamp_delta_min = min_tx_timestamp_delta;
//   tx_count = number_of_TX_packets_sent_this_second;
// }

// bool radio__note_successful_tx(void)
// {
//   if (number_of_TX_packets_sent_this_second != 0)
//   {
//     // First TX this measurement period
//     uint32_t current_time = scheduler__get_microsecond_ticks();
//     uint32_t tx_delta = current_time - last_tx_timestamp_micros;

//     if (tx_delta > stereo_or_mono_config->acceptable_tx_microsecond_delta_upper)
//     {
//       // max_tx_timestamp_delta = tx_delta;
//       //  printf("-  Max Delta Violation: %u  Max: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
//       bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//       bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
//       bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//       bad_timestamps_count++;
//       // printf("-  New Max Delta: %u \n", (unsigned int)max_tx_timestamp_delta);
//     }
//     else if (tx_delta < stereo_or_mono_config->acceptable_tx_microsecond_delta_lower)
//     {
//       // min_tx_timestamp_delta = tx_delta;
//       //  printf("-  Min Delta Violation: %u  Min: %u\n", (unsigned int)tx_delta, (unsigned int)upper_valid_tx_delta_limit);
//       bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//       bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = tx_delta;
//       bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//       bad_timestamps_count++;
//       // printf("-  New Min Delta: %u \n", (unsigned int)min_tx_timestamp_delta);
//     }

//     if (bad_timestamps_count > BAD_TIMESTAMPS_SIZE)
//     {
//       printf("-  Bad Timestamps Limit Reached: \n");
//       for (uint32_t i = 0; i < bad_timestamps_count; i++)
//       {
//         printf("  %u -- Seq: %u - Delta: %u - Timestamp: %u\n",
//                (unsigned int)i,
//                (unsigned int)bad_timestamps[i].sequence_number,
//                (unsigned int)bad_timestamps[i].timestamp_delta_from_previous,
//                (unsigned int)bad_timestamps[i].tx_timestamp);
//       }
//       state_machine__force_state_machine_error();
//       // assert(0);
//     }

//     // if (tx_delta > max_tx_timestamp_delta)
//     // {
//     //   max_tx_timestamp_delta = tx_delta;
//     //   bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//     //   bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = max_tx_timestamp_delta;
//     //   bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//     //   bad_timestamps_count++;
//     //   // printf("-  New Max Delta: %u \n", (unsigned int)max_tx_timestamp_delta);
//     // }
//     // else if (tx_delta < min_tx_timestamp_delta)
//     // {
//     //   min_tx_timestamp_delta = tx_delta;
//     //   bad_timestamps[bad_timestamps_count].sequence_number = next_sequence_number;
//     //   bad_timestamps[bad_timestamps_count].timestamp_delta_from_previous = max_tx_timestamp_delta;
//     //   bad_timestamps[bad_timestamps_count].tx_timestamp = current_time;
//     //   bad_timestamps_count++;
//     //   // printf("-  New Min Delta: %u \n", (unsigned int)min_tx_timestamp_delta);
//     // }
//   }
//   else
//   {
//     // first measurement in series. Have nothing to compare it against
//     max_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
//     min_tx_timestamp_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
//     // printf("-  New Max Delta: %u. , New Min Delta : %u\n", (unsigned int)max_tx_timestamp_delta, (unsigned int)min_tx_timestamp_delta);
//   }
//   last_tx_timestamp_micros = scheduler__get_microsecond_ticks();
//   number_of_TX_packets_sent_this_second++;
// }

// bool radio__note_retry_sent(void)
// {
//   // get TX timestamp
// }

// void radio__validate_radio_statistics(void)
// {
//   // TODO CRITICAl SECTION??
//   static uint32_t last_validation_timestamp_millis = 0;

//   // validate period
//   if (((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) >= 999) && ((scheduler__get_millisecond_ticks() - last_validation_timestamp_millis) <= 1001))
//   {
//     bool good_count = radio__validate_radio_tx_count();
//     bool good_deltas = radio__validate_radio_tx_timestamp_deltas();

//     printf("-  Max Delta: %u. , Min Delta : %u\n", (unsigned int)max_tx_timestamp_delta, (unsigned int)min_tx_timestamp_delta);
//     printf("-  TX Count : %u\n", (unsigned int)number_of_TX_packets_sent_this_second);
//     if (bad_timestamps_count > 0)
//     {
//       printf("-  Bad Timestamps : \n");
//       for (uint32_t i = 0; i < bad_timestamps_count; i++)
//       {
//         printf("   -- Seq: %u - Delta: %u - Timestamp: %u\n",
//                (unsigned int)bad_timestamps[i].sequence_number,
//                (unsigned int)bad_timestamps[i].timestamp_delta_from_previous,
//                (unsigned int)bad_timestamps[i].tx_timestamp);
//       }
//     }
//     if (good_count == false)
//     {
//       // if number_of_TX_packets_sent_this_second
//     }
//   }
//   else
//   {
//     printf("-  Measurement Period Start : %u\n", (unsigned int)last_validation_timestamp_millis);
//     printf("-  Measurement Period End : %u\n", (unsigned int)scheduler__get_millisecond_ticks());
//     printf("-  Measurement Period Time : %u\n", (unsigned int)(scheduler__get_millisecond_ticks() - last_validation_timestamp_millis));
//     // TODO HANDLE ELSE
//   }

//   last_validation_timestamp_millis = scheduler__get_millisecond_ticks();
//   radio__reset_radio_statistics();
// }

// void radio__reset_radio_statistics(void)
// {
//   number_of_TX_packets_sent_this_second = 0;
//   max_tx_timestamp_delta = 0;
//   min_tx_timestamp_delta = 0;
//   memset(&(bad_timestamps[0]), 0, sizeof(bad_timestamps));
//   bad_timestamps_count = 0;
//   moving_average_tx_delta = stereo_or_mono_config->expected_tx_microsecond_delta;
// }

// void radio__set_control_bit(uint8_t *control_bits, uint8_t bit)
// {
//   *control_bits |= bit;
// }
// void radio__reset_control_bit(uint8_t *control_bits, uint8_t bit)
// {
//   *control_bits &= bit;
// }

// void radio_buffer_assert(void)
// {
//   printf("Time: %u - ", (unsigned int)scheduler__get_millisecond_ticks());
//   printf("Assert \n");
//   printf("Left Index : %u\n", (unsigned int)active_buffer_index_left);
//   printf("Right Index : %u\n", (unsigned int)active_buffer_index_right);
//   printf("Printing Buffers \n");
//   for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
//   {
//     printf("-- %u - Used : %u , ready_to_send : %u , left_present : %u , right_present : %u, sequence : %u\n",
//            (unsigned int)i,
//            (unsigned int)radio_packet_buffer[i].used,
//            (unsigned int)radio_packet_buffer[i].ready_to_be_sent,
//            (unsigned int)radio_packet_buffer[i].left_data_present,
//            (unsigned int)radio_packet_buffer[i].right_data_present,
//            (unsigned int)radio_packet_buffer[i].payload.header.sequence_number);
//   }
//   printf("Current Sequence %u\n", (unsigned int)next_sequence_number);
//   printf("\nCounters \n");
//   counters__one_second_print();
//   for (uint32_t i = 0; i < 100; i++)
//   {
//     counters__run_debug_print_state_machine();
//   }
// }

// void radio__init_packet_buffers(void)
// {
//   memset(radio_packet_buffer, 0, sizeof(radio_packet_buffer));
//   next_sequence_number = 0;
//   sending_packet = false;
//   active_buffer_index_left = 0;
//   active_buffer_index_right = 0;
//   left_data_received_count = 0;
//   right_data_received_count = 0;
//   number_of_TX_packets_sent_this_second = 0;
//   max_tx_timestamp_delta = 0;
//   min_tx_timestamp_delta = 0;
//   stereo_or_mono_config = &mono_config;
//   radio_retry__init();
// }

// void radio__increment_sequence_number(void)
// {
//   if (next_sequence_number == 0xFFFF)
//   {
//     next_sequence_number = 0;
//   }
//   else
//   {
//     next_sequence_number++;
//   }

// #define DEBUG_TEST_MISSING_SEQUENCE_NUMBER 1 // forces a sequence number to be skipped
// #if (DEBUG_TEST_MISSING_SEQUENCE_NUMBER == 1)
//   if (next_sequence_number == 1000)
//   {
//     next_sequence_number++;
//   }
// #endif
// }

// static uint32_t radio__create_new_packet_buffer(void)
// {
//   if (radio_packet_buffer[packet_buffer_head].used == true)
//   {
//     if ((radio_packet_buffer[packet_buffer_head].ready_to_be_sent == true) && (radio_packet_buffer[packet_buffer_head].sent == false))
//     {
//       debug__increment_packet_buffer_overflows();
//       // TODO Handle overflow
//       DEBUG_STATES_LOG(printf("Forcing Error Due To Oerflows\n"));
//       state_machine__force_state_machine_error();
//     }
//     else if ((radio_packet_buffer[packet_buffer_head].ready_to_be_sent == false) && (radio_packet_buffer[packet_buffer_head].sent == true))
//     {
//       // This should be impossible, somethign seriously wrong. Throw error
//     }
//     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("No Empty Buffer, Overwriting for Sequence: %u\n", (unsigned int)next_sequence_number));

//     memset(&(radio_packet_buffer[packet_buffer_head]), 0, sizeof(packet_buffer_t));
//   }

//   uint32_t return_index = packet_buffer_head++;
//   radio_packet_buffer[return_index].payload.header.sequence_number = next_sequence_number;
//   debug__set_end_sequence_number(next_sequence_number);
//   radio__increment_sequence_number();
//   radio_packet_buffer[return_index].used = true;

//   if (packet_buffer_head >= NUMBER_OF_PACKET_BUFFERS)
//   {
//     packet_buffer_head = 0;
//   }
//   return return_index;
// }

// // static uint32_t radio__create_new_packet_buffer(void)
// // {
// //   //  printf("\n");

// //   for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
// //   {
// //     if (radio_packet_buffer[i].used == false)
// //     {
// //       radio_packet_buffer[i].payload.header.sequence_number = next_sequence_number;
// //       debug__set_sequence_number(next_sequence_number);
// //       increment_sequence_number();
// //       radio_packet_buffer[i].used = true;
// //       return i;
// //     }
// //   }

// //   DEBUG_PACKETS_LOG(debug__audio_buffer_printf("No Free Buffer, Overwriting for Sequence: %u\n", (unsigned int)next_sequence_number));

// //   uint32_t oldest_packet_index = 0xFFFFFFFF;
// //   uint16_t oldest_packet_sequence_number = 0xFFFF;

// //   for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
// //   {
// //     if (radio_packet_buffer[i].payload.header.sequence_number < oldest_packet_sequence_number)
// //     {
// //       oldest_packet_sequence_number = radio_packet_buffer[i].payload.header.sequence_number;
// //       oldest_packet_index = i;
// //     }
// //     //        }
// //   }

// //   if (oldest_packet_index != 0xFFFFFFFF)
// //   {
// //     if (radio_packet_buffer[oldest_packet_index].ready_to_be_sent == true)
// //     {
// //       debug__increment_packet_buffer_overflows();
// //     }

// //     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Overwriting Buffer: %u \n", (unsigned int)oldest_packet_index));
// //     memset(&(radio_packet_buffer[oldest_packet_index]), 0, sizeof(packet_buffer_t));
// //     radio_packet_buffer[oldest_packet_index].payload.header.sequence_number = next_sequence_number;
// //     debug__set_sequence_number(next_sequence_number);
// //     increment_sequence_number();
// //     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Next Sequence : %u\n", (unsigned int)next_sequence_number));
// //     radio_packet_buffer[oldest_packet_index].used = true;
// //     return oldest_packet_index;
// //   }

// //   // ERROR BUFFER ISN"T BEING CLEANED OUT, TX FAILING FOR WHATEVER REASON
// //   radio__error_handle();
// //   // radio_buffer_assert();
// //   // assert(0);

// //   return 0xFFFFFFFF;
// // }

// bool radio__indicate_data_buffer_was_filled_by_pointer(bool left_or_right_data)
// {
//   if ((radio_packet_buffer[active_buffer_index_left].left_data_present == true) && (left_or_right_data == LEFT))
//   {
//     // ERROR shouldn't be possible, LEFT data Already Present
//     DEBUG_PACKETS_LOG(printf("ERROR Got data LEFT but index %u already has present_flag set\n - ", (unsigned int)active_buffer_index_left));
//     radio__error_handle();
//     //      radio_buffer_assert();
//     //      assert(0);
//   }
//   else if ((radio_packet_buffer[active_buffer_index_right].right_data_present == true) && (left_or_right_data == RIGHT))
//   {
//     // ERROR shouldn't be possible, RIGHT data Already Present
//     DEBUG_PACKETS_LOG(printf("ERROR Got data RIGHT but index %u already has present_flag set\n - ", (unsigned int)active_buffer_index_right));
//     radio__error_handle();
//     //      radio_buffer_assert();
//     //      assert(0);
//   }

//   if (left_or_right_data == RIGHT)
//   {

//     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Got Right, set flag index %u\n", (unsigned int)active_buffer_index_right));

//     radio_packet_buffer[active_buffer_index_right].right_data_present = true;
//     right_data_received_count++;
//     if ((radio_packet_buffer[active_buffer_index_right].left_data_present == true) && (radio_packet_buffer[active_buffer_index_right].right_data_present == true))
//     {
//       radio_packet_buffer[active_buffer_index_right].ready_to_be_sent = true;
//       DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Buffer %u filled\n", (unsigned int)active_buffer_index_right));
//     }
//   }
//   else
//   {
// #ifdef DEBUG_RADIO
//     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Got Left, set flag index %u\n", (unsigned int)active_buffer_index_left));
// #endif
//     radio_packet_buffer[active_buffer_index_left].left_data_present = true;
//     left_data_received_count++;
//     if ((radio_packet_buffer[active_buffer_index_left].left_data_present == true) && (radio_packet_buffer[active_buffer_index_left].right_data_present == true))
//     {
//       radio_packet_buffer[active_buffer_index_left].ready_to_be_sent = true;
// #ifdef DEBUG_RADIO
//       DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Buffer %u filled\n", (unsigned int)active_buffer_index_left));
// #endif
//     }
//   }

//   //  if(((radio_packet_buffer[active_buffer_index].left_data_present == true) && (left_or_right_data == LEFT)) || ((radio_packet_buffer[active_buffer_index].right_data_present == true) && (left_or_right_data == RIGHT)))
//   //    {
//   //      //ERROR shouldn't be possible
//   //      printf("ERROR Got data %u\n - ", (unsigned int)left_or_right_data);
//   //  radio__error_handle();
//   //      radio_buffer_assert();
//   //      assert(0);
//   //    }
//   //
//   //  if ((radio_packet_buffer[active_buffer_index].used != true) && (radio_packet_buffer[active_buffer_index].ready_to_be_sent == true))
//   //    {
//   //      //ERROR shouldn't be possible
//   //  radio__error_handle();
//   //      radio_buffer_assert();
//   //      assert(0);
//   //    }

//   //  if (left_or_right_data == RIGHT)
//   //    {
//   //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//   //      printf("Got Right\n");
//   //      radio_packet_buffer[active_buffer_index].right_data_present = true;
//   //    }
//   //  else
//   //    {
//   //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//   //      printf("Got Left\n");
//   //      radio_packet_buffer[active_buffer_index].left_data_present = true;
//   //    }
//   //
//   //  if ((radio_packet_buffer[active_buffer_index].left_data_present == true) && (radio_packet_buffer[active_buffer_index].right_data_present == true))
//   //    {
//   //      radio_packet_buffer[active_buffer_index].ready_to_be_sent = true;
//   //      partially_filled_buffer_exists = false;
//   //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//   //      printf("Buffer %u filled\n", (unsigned int)active_buffer_index);
//   //    }
//   return false;
// }

// uint8_t *radio__get_pointer_to_data_buffer_channel(bool left_or_right_data)
// {

//   if ((radio_packet_buffer[active_buffer_index_left].used != true) && (radio_packet_buffer[active_buffer_index_right].used != true))
//   {
//     // Creating First Buffer
//     DEBUG_PACKETS_LOG(printf("Creating First Buffer\n"));

//     uint32_t new_buffer_index = radio__create_new_packet_buffer();
//     DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Making New Buffer %u\n", (unsigned int)new_buffer_index));

//     active_buffer_index_left = new_buffer_index;
//     active_buffer_index_right = new_buffer_index;
//   }

//   if ((radio_packet_buffer[active_buffer_index_left].left_data_present == false) && (left_or_right_data == LEFT))
//   {
//     return (&(radio_packet_buffer[active_buffer_index_left].payload.data_left[0]));
//   }
//   else if ((radio_packet_buffer[active_buffer_index_right].right_data_present == false) && (left_or_right_data == RIGHT))
//   {
//     return (&(radio_packet_buffer[active_buffer_index_right].payload.data_right[0]));
//   }

//   // if it gets here, the buffer isn't free
//   // check if the other buffer has a free slot

//   if (left_or_right_data == LEFT)
//   {
//     if (radio_packet_buffer[active_buffer_index_right].left_data_present != true)
//     {
//       active_buffer_index_left = active_buffer_index_right;
//       return (&(radio_packet_buffer[active_buffer_index_left].payload.data_left[0]));
//     }
//   }
//   else
//   {
//     if (radio_packet_buffer[active_buffer_index_left].right_data_present != true)
//     {
//       active_buffer_index_right = active_buffer_index_left;
//       return (&(radio_packet_buffer[active_buffer_index_right].payload.data_right[0]));
//     }
//   }

//   // if it gets here, the other channel's buffer isn't free
//   // make a new buffer

//   uint32_t new_buffer_index = radio__create_new_packet_buffer();
//   DEBUG_PACKETS_LOG(debug__audio_buffer_printf("Making New Buffer %u\n", (unsigned int)new_buffer_index));

//   if (left_or_right_data == LEFT)
//   {
//     // LEFT
//     active_buffer_index_left = new_buffer_index;
//     return (&(radio_packet_buffer[active_buffer_index_left].payload.data_left[0]));
//   }
//   else
//   {
//     // RIGHT
//     active_buffer_index_right = new_buffer_index;
//     return (&(radio_packet_buffer[active_buffer_index_right].payload.data_right[0]));
//   }

//   //  if (radio_packet_buffer[active_buffer_index].left_data_present && (left_or_right_data == LEFT))
//   //    {
//   //      active_buffer_index = radio__create_new_packet_buffer();
//   //      if (active_buffer_index == 0xFFFFFFFF)
//   //        {
//   //          //ERROR
//   //  radio__error_handle();
//   //          radio_buffer_assert();
//   //          assert(0);
//   //        }
//   //      else
//   //        {
//   //          partially_filled_buffer_exists = true;
//   //        }
//   //      //    }
//   //
//   //
//   //      if (left_or_right_data == RIGHT)
//   //        {
//   //          return (&(radio_packet_buffer[active_buffer_index].payload.data_right[0]));
//   //        }
//   //      else
//   //        {
//   //          return (&(radio_packet_buffer[active_buffer_index].payload.data_left[0]));
//   //        }
//   //    }

//   // void radio__add_channel_data_to_buffer(uint8_t* new_data_pointer, bool left_or_right_data)
//   //{
//   //   if (partially_filled_buffer_exists == false)
//   //     {
//   //       active_buffer_index = radio__create_new_packet_buffer();
//   //       if (active_buffer_index == 0xFFFFFFFF)
//   //         {
//   //           //ERROR
//   //   radio__error_handle();
//   //           radio_buffer_assert();
//   //           assert(0);
//   //         }
//   //       else
//   //         {
//   //           partially_filled_buffer_exists = true;
//   //         }
//   //     }
//   //
//   //   if (partially_filled_buffer_exists != true)
//   //     {
//   //       //ERROR
//   //   radio__error_handle();
//   //       radio_buffer_assert();
//   //       assert(0);
//   //     }
//   //
//   //   if(((radio_packet_buffer[active_buffer_index].left_data_present == true) && (left_or_right_data == LEFT)) || ((radio_packet_buffer[active_buffer_index].right_data_present == true) && (left_or_right_data == RIGHT)))
//   //     {
//   //       //ERROR shouldn't be possible
//   //     }
//   //
//   //   if (left_or_right_data == RIGHT)
//   //     {
//   //       memcpy(&(radio_packet_buffer[active_buffer_index].payload.data_right), new_data_pointer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);
//   //       radio_packet_buffer[active_buffer_index].right_data_present = true;
//   //     }
//   //   else
//   //     {
//   //       memcpy(&(radio_packet_buffer[active_buffer_index].payload.data_right), new_data_pointer, RADIO_PACKET_DATA_SIZE_PER_CHANNEL);
//   //       radio_packet_buffer[active_buffer_index].left_data_present = true;
//   //     }
//   //
//   //
//   //   if ((radio_packet_buffer[active_buffer_index].left_data_present == true) && (radio_packet_buffer[active_buffer_index].right_data_present == true))
//   //     {
//   //       radio_packet_buffer[active_buffer_index].ready_to_be_sent = true;
//   //       partially_filled_buffer_exists = false;
//   //     }
// }

// static void radio__mark_packet_as_sent(void)
// {
  
//   radio_packet_buffer[tx_packet_in_flight_info.packet_buffer_index].sent = true;

//   if (tx_packet_in_flight_info.retry == true)
//   {
//     radio_retry__note_retry_packet_successfully_sent(radio_packet_buffer[tx_packet_in_flight_info.packet_buffer_index].payload.header.sequence_number);
//   }
//   memset(&tx_packet_in_flight_info,0,sizeof(tx_packet_in_flight_info_t));

//   // for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
//   // {
//   //   if (radio_packet_buffer[i].payload.header.sequence_number == sequence_number)
//   //   {
//   //     // radio_packet_buffer[i].ready_to_be_sent = false;
//   //     radio_packet_buffer[i].sent = true;
//   //     //          printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//   //     //          printf("Marked buffer %u free , (unsigned int)sequence : %u\n", (unsigned int)i, sequence_number);
//   //     return true;
//   //   }
//   // }
//   // //  printf("***Failed to mark buffer free , sequence : %u\n", (unsigned int)sequence_number);
//   // return false;
// }

// static bool radio__check_if_any_packet_ready_to_send(uint32_t *index)
// {
//   uint32_t oldest_packet_index = 0xFFFFFFFF;
//   uint16_t oldest_packet_sequence_number = 0xFFFF;
//   // ToDo : Fix for sequence number overflow

//   for (uint32_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
//   {
//     if ((radio_packet_buffer[i].used == true) && (radio_packet_buffer[i].ready_to_be_sent == true) && (radio_packet_buffer[i].sent == false))
//     {
//       if (radio_packet_buffer[i].payload.header.sequence_number < oldest_packet_sequence_number)
//       {
//         oldest_packet_sequence_number = radio_packet_buffer[i].payload.header.sequence_number;
//         oldest_packet_index = i;
//       }
//     }
//   }

//   if (oldest_packet_index != 0xFFFFFFFF)
//   {
//     *index = oldest_packet_index;
//     return true;
//   }
//   return false;
// }

// static uint32_t radio__find_packet_buffer_index_by_sequence_number(uint16_t sequence_number)
// {
//   for (uint8_t i = 0; i < NUMBER_OF_PACKET_BUFFERS; i++)
//   {
//     if (radio_packet_buffer[i].payload.header.sequence_number == sequence_number)
//     {
//       return i;
//     }
//   }
//   return 0xFFFFFFFF;
// }

// bool radio__send_packet_by_sequence_number(uint16_t sequence_number, bool retry)
// {
//   uint32_t buffer_index = radio__find_packet_buffer_index_by_sequence_number(sequence_number);
//   if (buffer_index > NUMBER_OF_PACKET_BUFFERS)
//   {
//     printf("Buffer Index Too High\n");
//     return false;
//   }

//   // printf("Sending Packet : %u, Index : %u\n", (unsigned int)radio_packet_buffer[buffer_index].payload.header.sequence_number, (unsigned int)buffer_index);

//   radio__send_packet_new(buffer_index,retry);
// }

// static bool radio__send_packet_new(uint32_t buffer_index, bool retry)
// {
//   if (buffer_index > NUMBER_OF_PACKET_BUFFERS)
//   {
//     printf("Buffer Index Too High\n");
//     return false;
//   }

//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//   uint32_t bytes_loaded = 0;
//   bool return_value = false;


//   header_t new_header_copy;
//   memcpy(&new_header_copy,&(radio_packet_buffer[buffer_index].payload.header),sizeof(header_t));
//   new_header_copy.control_bits |= CONTROL_BITS__RETRY;

//   uint32_t return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(new_header_copy)), RADIO_PACKET_HEADER_SIZE, true);
//   if (return_count != RADIO_PACKET_HEADER_SIZE)
//   {
//     printf("Write Header To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[buffer_index].payload.data_left)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
//   if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
//   {
//     printf("Write Left Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[buffer_index].payload.data_right)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
//   if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
//   {
//     printf("Write Right Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   // printf("Loaded to Buffer Successfully\n");

//   CORE_DECLARE_IRQ_STATE;
//   CORE_ENTER_CRITICAL();

//   RAIL_Status_t RAIL_StartTx_return = RAIL_StartTx(rail_handle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
//   if (RAIL_StartTx_return == SL_STATUS_OK)
//   {
//     tx_packet_in_flight_info.packet_buffer_index = buffer_index;
//     tx_packet_in_flight_info.retry = retry;
//     debug__increment_number_of_TX_attempts(retry);
//     sending_packet = true;
//     //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//     //  printf("Sent buffer %u , sequence : %u\n", (unsigned int)buffer_index, radio_packet_buffer[buffer_index].payload.header.sequence_number);
//     return_value = true;
//   }
//   else
//   {
//     debug__increment_number_of_TX_failed(retry);
//     //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//      printf("Failed to Send buffer %u , sequence : %u, status: 0x%X\n", (unsigned int)buffer_index, radio_packet_buffer[buffer_index].payload.header.sequence_number, (unsigned int)RAIL_StartTx_return);
//     CORE_EXIT_CRITICAL();
//     return false;
//   }
//   CORE_EXIT_CRITICAL();

//    //printf("Bytes Loaded: %u\n", (unsigned int)bytes_loaded);

//   return return_value;
// }

// static bool radio__send_packet(uint32_t index_to_send)
// {
//   bool return_value = false;
//   //  printf("Attempting to send packet from buffer %u\n", (unsigned int)index_to_send);

//   uint32_t bytes_loaded = 0;

//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

//   radio_packet_buffer[index_to_send].payload.header.size = RADIO_PACKET_DATA_SIZE - 1;

//   if (adc__get_audio_stereo_flag() == false)
//   {
//     radio__set_control_bit(&radio_packet_buffer[index_to_send].payload.header.control_bits, CONTROL_BITS__STEREO);
//     // radio_packet_buffer(radio_packet_buffer[index_to_send].payload.header.control_bits |= CONTROL_BITS__STEREO;
//   }
//   else
//   {
//     radio__reset_control_bit(&radio_packet_buffer[index_to_send].payload.header.control_bits, CONTROL_BITS__STEREO);
//     // radio_packet_buffer[index_to_send].payload.header.control_bits &= (~CONTROL_BITS__STEREO);
//   }

//   //  printf("Sent 0x%u:\n", (unsigned int)radio_packet_buffer[index_to_send].payload.header.sequence_number);

//   uint32_t return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[index_to_send].payload.header)), RADIO_PACKET_HEADER_SIZE, true);
//   if (return_count != RADIO_PACKET_HEADER_SIZE)
//   {
//     printf("Write Header To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   // return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[index_to_send].payload.header)), RADIO_PACKET_HEADER_SIZE, true);
//   // if (return_count != RADIO_PACKET_HEADER_SIZE)
//   // {
//   //   printf("Write Header To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//   //   radio__error_handle();
//   //   //      assert(0);
//   //   return false;
//   // }

//   return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[index_to_send].payload.data_left)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
//   if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
//   {
//     printf("Write Left Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   return_count = RAIL_WriteTxFifo(rail_handle, (uint8_t *)(&(radio_packet_buffer[index_to_send].payload.data_right)), RADIO_PACKET_DATA_SIZE_PER_CHANNEL, false);
//   if (return_count != RADIO_PACKET_DATA_SIZE_PER_CHANNEL)
//   {
//     printf("Write Right Data To TX FIFO Failed: 0x%u:\n", (unsigned int)return_count);
//     radio__error_handle();
//     //      assert(0);
//     return false;
//   }
//   bytes_loaded += return_count;

//   // uint16_t TX_Bytes = RADIO_FIFO_SIZE - RAIL_GetTxFifoSpaceAvailable(rail_handle);
//   // if (TX_Bytes != RADIO_PAYLOAD_SIZE)
//   // {
//   //   assert(0);
//   //   return false;
//   // }

//   //  uint16_t RAIL_TXFIFO_LENGTH_SIZE = RAIL_SetFixedLength(rail_handle, RADIO_PAYLOAD_SIZE);
//   //   if (RAIL_TXFIFO_LENGTH_SIZE != RADIO_PAYLOAD_SIZE)
//   //   {
//   //     assert(0);
//   //   }

//   CORE_DECLARE_IRQ_STATE;
//   CORE_ENTER_CRITICAL();

//   RAIL_Status_t RAIL_StartTx_return = RAIL_StartTx(rail_handle, channel, RAIL_TX_OPTIONS_DEFAULT, NULL);
//   if (RAIL_StartTx_return == SL_STATUS_OK)
//   {
//     tx_packet_in_flight_info.packet_buffer_index = index_to_send;
//     debug__increment_number_of_TX_attempts(false);
//     sending_packet = true;
//     //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//     // printf("Sent buffer %u , (unsigned int)sequence : %u\n", (unsigned int)index_to_send, radio_packet_buffer[index_to_send].payload.header.sequence_number);
//     return_value = true;
//   }
//   else
//   {
//     debug__increment_number_of_TX_failed(false);
//     //      printf("Time: %u - ", (unsigned int)get_millisecond_ticks());
//      //printf("Failed to Send buffer %u , sequence : %u, status: 0x%X\n", (unsigned int)index_to_send, radio_packet_buffer[index_to_send].payload.header.sequence_number, (unsigned int)status);
//     CORE_EXIT_CRITICAL();
//     return false;
//   }
//   CORE_EXIT_CRITICAL();

//    //printf("Bytes Loaded: %u\n", (unsigned int)bytes_loaded);

//   return return_value;
// }

// static void radio__try_to_send_a_packet(void)
// {
//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//   RAIL_RadioState_t state = RAIL_GetRadioState(rail_handle);
//   if (((state == RAIL_RF_STATE_RX) || (state == RAIL_RF_STATE_IDLE) || (state == RAIL_RF_STATE_INACTIVE)) && (sending_packet == false))
//   {
//     uint32_t index_of_available_packet = 0xFFFFFFFF;
//     if (radio__check_if_any_packet_ready_to_send(&index_of_available_packet) == true)
//     {
//       radio__send_packet(index_of_available_packet);
//     }
//   }
// }

// bool radio__is_radio_busy(void)
// {
//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//     RAIL_RadioState_t state = RAIL_GetRadioState(rail_handle);
//     if (((state == RAIL_RF_STATE_RX) || (state == RAIL_RF_STATE_IDLE) || (state == RAIL_RF_STATE_INACTIVE)))
//     {
//       return false;
//     }
// }

// void radio__run_process(void)
// {
//   // if (radio_retry__run_process())
//   // {
//   //   //return;
//   // }
//   radio_retry__run_process();
//   radio__try_to_send_a_packet();
// }

// void radio_process_receiver_packet(RAIL_Handle_t rail_handle)
// {
//   RAIL_RxPacketHandle_t rx_packet_handle;
//   RAIL_RxPacketInfo_t packet_info;
//   //  uint8_t *start_of_packet = 0;

//   rx_packet_handle = RAIL_GetRxPacketInfo(rail_handle, RAIL_RX_PACKET_HANDLE_OLDEST_COMPLETE, &packet_info);

//   payload_t temp;

//   RAIL_CopyRxPacket((uint8_t *)&temp, &packet_info);

//   // debug__audio_buffer_printf("Packet : %u\n");
//   // for (uint32_t i = 0; i < temp.header.size + 4 + 1; i++)
//   // {
//   //   debug__audio_buffer_printf("0x%X, ", (unsigned int)((uint8_t *)(&temp))[i]);
//   // }
//   uint32_t number_of_missing_packets_requested = (temp.header.size + 1) >> 1;

//   // debug__audio_buffer_printf("\nNumber Sequence Numbers Requested : %u\n", number_of_missing_packets_requested);
//   for (uint32_t i = 0; i < number_of_missing_packets_requested; i++)
//   {
//     uint16_t missing_sequence_number = (temp.data_left[(i * 2)] << 8) | (temp.data_left[(i * 2) + 1]);
//     radio_retry__add_missing_packet_entry(missing_sequence_number);
//     debug__audio_buffer_printf("%u - Got Request For Sequence Number : %u\n", i, missing_sequence_number);
//   }
// }

// bool radio__process_event_rx(RAIL_Handle_t rail_handle, RAIL_Events_t events)
// {
//   (void)rail_handle;
//   (void)events;
//   if (events & RAIL_EVENTS_RX_COMPLETION)
//   {
//     // debug__audio_buffer_printf("Got RX \n");
//     //      if (logged_events_count < LOGGED_EVENTS_BUFFER_SIZE)
//     //        {
//     //          logged_events[logged_events_count++] = events;
//     //        }
//     // printf("%s\n", getString(events));
//     if (events & RAIL_EVENT_RX_PACKET_RECEIVED)
//     {
//       radio_process_receiver_packet(rail_handle);
//       // process_packet(rail_handle);
//     }
//     if (events & RAIL_EVENT_RX_PACKET_ABORTED)
//     {
//       // debug_increment_number_of_RX_abort();
//     }
//     if (events & RAIL_EVENT_RX_FRAME_ERROR)
//     {
//       // debug_increment_number_of_RX_error();
//     }
//     if (events & RAIL_EVENT_RX_FIFO_OVERFLOW)
//     {
//       // debug_increment_number_of_RX_overflow();
//     }
//     if (events & RAIL_EVENT_RX_ADDRESS_FILTERED)
//     {
//       // debug_increment_number_of_RX_filterred();
//     }
//     if (events & RAIL_EVENT_RX_SCHEDULED_RX_MISSED)
//     {
//       // debug_increment_number_of_RX_missed();
//     }
//     return true;
//   }
//   return false;
// }

// bool radio__process_event_tx(RAIL_Handle_t rail_handle, RAIL_Events_t events)
// {
//   (void)rail_handle;
//   (void)events;
//   if (events & RAIL_EVENT_TX_PACKET_SENT)
//   {
//     // GPIO_PinOutToggle(gpioPortB, 3);
//     // set_rgb(1, green, 0, 127);

//     radio__mark_packet_as_sent();
//     radio__note_successful_tx();
//     //     RAIL_TxPacketDetails_t temp_RAIL_TxPacketDetails_t;

//     //     RAIL_Status_t RAIL_GetTxPacketDetails_status = RAIL_GetTxPacketDetails(rail_handle,&temp_RAIL_TxPacketDetails_t);
//     //     if (RAIL_GetTxPacketDetails_status != RAIL_STATUS_NO_ERROR)
//     //     {
//     //       printf("TX_ERROR??? : 0x%X\n",RAIL_GetTxPacketDetails_status);
//     //     }
//     // else
//     // {
//     //   uint32_t bytes_sent = temp_RAIL_TxPacketDetails_t.timeSent.totalPacketBytes;
//     //     printf("Bytes Sent: %u\n", (unsigned int)bytes_sent);
//     // }

//     uint32_t bytes_remaining = FIFO_Size - RAIL_GetTxFifoSpaceAvailable(rail_handle);
//     // printf("Fifo Size: %u , Available : %u , Bytes Remaining: %u , Bytes Sent %u\n",(unsigned int)FIFO_Size,(unsigned int)RAIL_GetTxFifoSpaceAvailable(rail_handle), (unsigned int)bytes_remaining, (unsigned int)(RADIO_PAYLOAD_SIZE - bytes_remaining));

//     debug__log_TX_success(tx_packet_in_flight_info.retry);
//     sending_packet = false;
//     return true;
//     // radio__try_to_send_a_packet();
//   }
//   else
//   {
//     if (events & RAIL_EVENTS_TX_COMPLETION)
//     {
//       sending_packet = false;
//       debug__log_TX_fail(tx_packet_in_flight_info.retry);
//       if (events & RAIL_EVENT_TX_ABORTED)
//       {
//         debug__log_TX_abort(tx_packet_in_flight_info.retry);
//       }
//       else if (events & RAIL_EVENT_TX_BLOCKED)
//       {
//         debug__log_TX_block(tx_packet_in_flight_info.retry);
//       }
//       else if (events & RAIL_EVENT_TX_UNDERFLOW)
//       {
//         debug__log_TX_underflow(tx_packet_in_flight_info.retry);
//       }
//       else if (events & RAIL_EVENT_TX_SCHEDULED_TX_MISSED)
//       {
//         debug__log_TX_missed(tx_packet_in_flight_info.retry);
//       }
//       else if (events & RAIL_EVENT_TX_CHANNEL_BUSY)
//       {
//         debug__log_TX_busy(tx_packet_in_flight_info.retry);
//       }
//       else
//       {
//         printf("UNHANDLED TX ERROR: 0x%u:\n", (long long unsigned int)events);
//       }
//       return true;
//     }
//   }
//   return false;
// }

// void radio__process_event(RAIL_Handle_t rail_handle, RAIL_Events_t events)
// {
//   (void)rail_handle;
//   (void)events;
//   if (radio__process_event_tx(rail_handle, events) == true)
//   {
//     return;
//   }
//   else if (radio__process_event_rx(rail_handle, events) == true)
//   {
//     return;
//   }

//   // logged_events[logged_events_count++] = events;
// }

// void radio__error_handle(void)
// {
//   printf("RADIO ERROR REINITIALIZING\n");
//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//   RAIL_Idle(rail_handle, RAIL_IDLE_FORCE_SHUTDOWN, false);

//   radio__radio_init();
// }

// void radio__radio_init(void)
// {
//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);

//   RAIL_ConfigEvents(rail_handle, RAIL_EVENTS_ALL, RAIL_EVENTS_ALL);

//   // uint16_t RAIL_TXFIFO_LENGTH_SIZE = RAIL_SetFixedLength(rail_handle, RADIO_PAYLOAD_SIZE);
//   // if (RAIL_TXFIFO_LENGTH_SIZE != RADIO_PAYLOAD_SIZE)
//   // {
//   //   assert(0);
//   // }
//   uint32_t RAIL_SetTxFifo_return = RAIL_SetTxFifo(rail_handle, (uint8_t *)tx_buffer, 0, RADIO_FIFO_SIZE);
//   if (RAIL_SetTxFifo_return < RADIO_PAYLOAD_SIZE)
//   {
//     printf("RAIL_SetTxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)RAIL_SetTxFifo_return);
//     assert(0);
//   }
//   printf("RAIL_SetTxFifo Success, %u Bytes\n", (unsigned int)RAIL_SetTxFifo_return);
//   FIFO_Size = RAIL_SetTxFifo_return;

//   RAIL_Status_t RAIL_SetTxPowerDbm_return = RAIL_SetTxPowerDbm(rail_handle, 190);
//   if (RAIL_SetTxPowerDbm_return != RAIL_STATUS_NO_ERROR)
//   {
//     printf("RAIL_SetTxPowerDbm failed, Status Code: %X\n", (unsigned int)RAIL_SetTxPowerDbm_return);
//     assert(0);
//   }
//   printf("RAIL_SetTxPowerDbm Success\n");

//   RAIL_Status_t RAIL_SetRxFifo_return = RAIL_SetRxFifo(rail_handle, (uint8_t *)RX_FIFO, &RX_FIFO_SIZE);
//   if (RAIL_SetRxFifo_return != RAIL_STATUS_NO_ERROR)
//   {
//     printf("RAIL_SetRxFifo failed, Status Code: %X\n", (unsigned int)RAIL_SetRxFifo_return);
//     assert(0);
//   }
//   if (RX_FIFO_SIZE != RADIO_FIFO_SIZE)
//   {
//     printf("RAIL_SetRxFifo Failed. Requested : %u Bytes, Got %u Bytes \n", (unsigned int)RADIO_FIFO_SIZE, (unsigned int)RX_FIFO_SIZE);
//     assert(0);
//   }
//   printf("RAIL_SetRxFifo Success, %u Bytes\n", (unsigned int)RX_FIFO_SIZE);

// #define ENABLE_PRS 0
// #if (ENABLE_PRS == 1)

// #define PRS_SOURCE PRS_ASYNC_CH_CTRL_SOURCESEL_RACL
// #define PRS_SIGNAL PRS_ASYNC_CH_CTRL_SIGSEL_RACLTX

//   CMU_ClockEnable(cmuClock_PRS, true);
//   CMU_ClockEnable(cmuClock_GPIO, true);
//   GPIO_PinOutSet(gpioPortB, 1);
//   PRS_SourceAsyncSignalSet(1,
//                            PRS_SOURCE,
//                            PRS_SIGNAL);
//   PRS_PinOutput(1,
//                 prsTypeAsync,
//                 gpioPortB,
//                 1);
// #endif
// }

// void radio__radio_deinit(void)
// {
//   RAIL_Handle_t *rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//   RAIL_ConfigEvents(rail_handle, 0, 0);
// }

// void radio__increment_channel(void)
// {
//   channel++;
//   if (channel == 20)
//   {
//     channel = 0;
//   }
//   channel_changed_flag = true;

//   RAIL_Handle_t rail_handle = sl_rail_util_get_handle(SL_RAIL_UTIL_HANDLE_INST);
//   RAIL_StartRx(rail_handle, channel, NULL);
// }

// uint32_t radio__get_channel(void)
// {
//   return channel;
// }