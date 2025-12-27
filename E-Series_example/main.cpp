#include "antsdrDevice.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <chrono>
#include "common.h"

#define TOTAL_SIZE (1024 * 1024 * 512) // 512MB
#define CHUNK_SIZE (4 * 1024 * 1024) // 4MB

int main(){
    size_t bytes_read;
    size_t one_block_sent = 0;
    size_t total_sent = 0;
    antsdrDevice device;
    device.set_ip_address("192.168.1.10");
    double gain = 65;
    
    if(device.open(true) < 0){
        printf("Failed to open device\n");
        return -1;
    }
    device.set_rx_samprate(61440000);
    device.set_tx_freq(5766.5e6);
    device.set_rx_freq(5766.5e6);
    device.set_rx_gain(gain, 1); // 1 ，2 ，3
    device.set_tx_samprate(61440000);
    device.set_tx_attenuation(20);
    
//// 录制----------------------------
#if 1
    device.create_socket_fd();
    device.config_recorder_data(CHUNK_SIZE*32);
    FILE *file = fopen("recorder_data.cs16", "wb");
    if (!file) {
        perror("Failed to open file for writing");
        device.release_socket();
        return -1;
    }
    device.recorder_data_file(file, CHUNK_SIZE*32);
    fclose(file);
    device.release_socket();
#endif

//// 回放----------------------------
#if 1
    device.create_socket_fd();
    device.sample_stop_replay();
    device.release_socket();

    device.create_socket_fd();
        
    FILE *file_r = fopen("recorder_data.cs16", "rb"); 
    if (!file_r) {
        perror("Failed to open file");
        device.release_socket();
        exit(EXIT_FAILURE); 
    }
    fseek(file_r, 0, SEEK_END);
    uint64_t file_size = ftell(file_r);
    rewind(file_r);
    int16_t blockcnt = file_size /CHUNK_SIZE;
    file_size= blockcnt*CHUNK_SIZE;
    device.send_replay_len(file_size);

    char *tx_buffer = (char*)malloc(CHUNK_SIZE);
    if (!tx_buffer) {
        printf("Failed to allocate memory for tx buffer\n");
        fclose(file_r);
        device.release_socket();
        exit(EXIT_FAILURE);
    }
    while ((bytes_read = fread(tx_buffer, 1, CHUNK_SIZE, file_r)) > 0) {
        one_block_sent = device.send_once_block_data(tx_buffer);
        total_sent += one_block_sent;
        if (one_block_sent == 0) {
            printf("Warning: send_once_block_data returned 0, this might indicate an error\n");
            break;
        }
        printf("total_sent:%lu,file_size:%lu\n",total_sent,file_size);
        if(total_sent == file_size)
            break;
    }
    fclose(file_r);
    free(tx_buffer);
    device.sample_start_replay();

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {

    }
    device.sample_stop_replay();
    device.release_socket();
#endif

//// 按键录制----------------------------
//// E310可使用，E200无此功能
#if 0
    device.create_socket_fd();
    printf("请按下按键触发录制\n");
    device.config_recorder_data_key(TOTAL_SIZE);
    FILE *file_key = fopen("recorder_data_key.cs16", "wb");
    if (!file_key) {
        perror("Failed to open file for writing");
        device.release_socket();
        return -1;
    }
    
    device.recorder_data_file(file_key, TOTAL_SIZE);
    fclose(file_key);
    device.release_socket();
    printf("Replay completed successfully.\n");
    
#endif
    fprintf(stdout,"Exit Program.\n");

    return 0;
}