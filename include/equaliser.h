#pragma once
#include "audio_stream.h"

void equaliser_init();
void equaliser_process_packet(AudioPacket* packet);
void equaliser_destroy();
