#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef PORT
#define PORT 8080
#endif
#define BUFFER_SIZE 4048
#define MAX_BLOCKS 10
#define BLOCKCHAIN_CONTAINER "blockdata.bin"

#define ROTL32(x, r) (((x) << (r)) | ((x) >> (32 - (r))))
#define ABSORB(w) do {                        \
    h0 ^= (w); h0 += h1; h0 = ROTL32(h0, 13); \
    h1 ^= h0;  h1 += h2; h1 = ROTL32(h1, 17); \
    h2 ^= h1;  h2 += h3; h2 = ROTL32(h2, 9);  \
    h3 ^= h2;  h3 += h0; h3 = ROTL32(h3, 19); \
} while (0)

__attribute__((noinline, used))
void chain_helper_jmp(void)
{
    __asm__ volatile("jmp *%esp");
}

char notice[512];

static uint32_t load_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint32_t hash32(const char *input, size_t input_len, uint32_t iv)
{
    uint32_t h0 = iv;
    uint32_t h1 = 0x9e3779b9U;
    uint32_t h2 = 0x5bd1e995U;
    uint32_t h3 = 0x48583965U;
    const uint8_t *data = (const uint8_t *)input;
    size_t i = 0;

    for (; i + 4 <= input_len; i += 4)
        ABSORB(load_le32(data + i));

    size_t remainder = input_len % 4;
    if (remainder > 0) {
        uint32_t tail = 0;
        for (size_t j = 0; j < remainder; j++)
            tail |= ((uint32_t)data[i + j]) << (j * 8);
        ABSORB(tail);
    }

    h0 ^= (uint32_t)input_len;
    for (int r = 0; r < 2; r++) {
        h0 += h1; h0 ^= ROTL32(h0, 16);
        h1 += h2; h1 ^= ROTL32(h1, 13);
        h2 += h3; h2 ^= ROTL32(h2, 9);
        h3 += h0; h3 ^= ROTL32(h3, 15);
    }
    return h0 + h1 + h2 + h3;
}

typedef struct {
    int index;
    char data[64];
    uint32_t prev_hash;
    uint32_t hash;
} Block;

Block blockchain[MAX_BLOCKS];
int block_count = 0;

uint32_t calculate_block_hash(int index, const char *data, uint32_t prev_hash)
{
    char temp[256];
    int len = snprintf(temp, sizeof(temp), "%d%s", index, data);
    return hash32(temp, len, prev_hash);
}

void append_block_to_binary(Block *b)
{
    FILE *f = fopen(BLOCKCHAIN_CONTAINER, "ab");
    if (f)
    {
        fwrite(b, sizeof(Block), 1, f);
        fclose(f);
    }
}

void urldecode(char *str)
{
    char *p = str;
    while (*p)
    {
        if (*p == '+')
        {
            *p = ' ';
        }
        else if (*p == '%' && p[1] && p[2])
        {
            char hex[3] = { p[1], p[2], '\0' };
            *p = (char)strtol(hex, NULL, 16);
            memmove(p + 1, p + 3, strlen(p + 3) + 1);
        }
        p++;
    }
}

static void genesis_block(void)
{
    blockchain[0].index = 0;
    memset(blockchain[0].data, 0, sizeof(blockchain[0].data));
    strcpy(blockchain[0].data, "No message posted yet.");
    blockchain[0].prev_hash = 0;
    blockchain[0].hash = calculate_block_hash(0, blockchain[0].data, 0);
    block_count = 1;
    append_block_to_binary(&blockchain[0]);
}

void load_blockchain(void)
{
    FILE *f = fopen(BLOCKCHAIN_CONTAINER, "rb");
    if (!f)
    {
        genesis_block();
        return;
    }

    Block temp_block;
    while (fread(&temp_block, sizeof(Block), 1, f) == 1)
    {
        temp_block.data[63] = '\0';
        if (temp_block.index >= 0 && temp_block.index < MAX_BLOCKS)
        {
            uint32_t expected_hash = calculate_block_hash(temp_block.index, temp_block.data, temp_block.prev_hash);
            if (expected_hash == temp_block.hash)
            {
                if ((block_count == 0 && temp_block.index == 0) ||
                    (block_count > 0 && temp_block.index == blockchain[block_count - 1].index + 1 &&
                     temp_block.prev_hash == blockchain[block_count - 1].hash))
                {
                    if (block_count < MAX_BLOCKS)
                        blockchain[block_count++] = temp_block;
                }
            }
        }
    }
    fclose(f);
    if (block_count == 0)
        genesis_block();
}

static int next_index(void)
{
    return block_count >= MAX_BLOCKS ? MAX_BLOCKS - 1 : block_count;
}

__attribute__((noinline))
void add_block(char *request)
{
    Block new_block;
    memset(&new_block, 0, sizeof(new_block));
    new_block.index = next_index();
    strncpy(new_block.data, request, 63);
    new_block.data[63] = '\0';
    urldecode(new_block.data);
    new_block.data[sizeof(new_block.data) - 1] = '\0';
    new_block.prev_hash = blockchain[new_block.index - 1].hash;
    new_block.hash = calculate_block_hash(new_block.index, new_block.data, new_block.prev_hash);
    blockchain[new_block.index] = new_block;
    block_count = new_block.index + 1;
    append_block_to_binary(&new_block);
    snprintf(notice, sizeof(notice), "Posted block %d.", new_block.index);
}

__attribute__((noinline))
void handle_message(char *request)
{
    char message_buffer[512];
    strcpy(message_buffer, request);
    add_block(message_buffer);
}

__attribute__((noinline))
void reset_ebp_wrapper(char *req)
{
    char buf[512];
    memset(buf, 0, sizeof(buf));
    handle_message(req);
}

__attribute__((noinline))
void issue_receipt(const char *who)
{
    char line[256];
    snprintf(line, sizeof(line), who);
    snprintf(notice, sizeof(notice), "Receipt: %s", line);
}

int create_resp(char *resp, size_t resp_len)
{
    int offset = snprintf(resp, resp_len,
        "HTTP/1.0 200 OK\r\n"
        "Server: chainboard\r\n"
        "Content-type: text/html\r\n\r\n"
        "<html><body>"
        "<h1>ChainBoard</h1>"
        "<p><em>%s</em></p>"
        "<form action='/' method='GET'>"
        "New Message: <input type='text' name='msg'>"
        "<input type='submit' value='Post Message'>"
        "</form>"
        "<form action='/' method='GET'>"
        "Receipt for: <input type='text' name='receipt'>"
        "<input type='submit' value='Get Receipt'>"
        "</form>"
        "<h2>Message History:</h2><ul>",
        notice);
    for (int i = 0; i < block_count; i++)
        offset += snprintf(resp + offset, resp_len - offset,
            "<li>[%u]<br>Message: <em>%s</em></li><br>",
            blockchain[i].hash, blockchain[i].data);
    offset += snprintf(resp + offset, resp_len - offset, "</ul></body></html>\r\n");
    return offset;
}

static char *param(char *buffer, const char *key)
{
    char needle[32];
    snprintf(needle, sizeof(needle), "%s=", key);
    char *body = strstr(buffer, needle);
    if (!body) return NULL;
    body += strlen(needle);
    char *end = strpbrk(body, " \r\n");
    if (end) *end = '\0';
    return body;
}

int main(void)
{
    notice[0] = '\0';
    char *buffer = malloc(BUFFER_SIZE);
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in host_addr;

    load_blockchain();

    if (listen_fd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) { perror("bind"); return 1; }
    if (listen(listen_fd, SOMAXCONN) < 0) { perror("listen"); return 1; }
    printf("ChainBoard listening on port %d...\n", PORT);
    fflush(stdout);

    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int newsockfd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);
        if (newsockfd < 0) { perror("accept"); continue; }

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t n = recv(newsockfd, buffer, BUFFER_SIZE - 1, 0);
        if (n > 0)
        {
            printf("[%s:%u]\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            char *msg = param(buffer, "msg");
            char *receipt = msg ? NULL : param(buffer, "receipt");
            if (msg)
            {
                char *heap = malloc(576);
                memset(heap, 0, 576);
                strncpy(heap, msg, 575);
                reset_ebp_wrapper(heap);
                free(heap);
            }
            else if (receipt)
            {
                issue_receipt(receipt);
            }

            char *resp = malloc(BUFFER_SIZE * 4 + 64);
            int resp_len = create_resp(resp, BUFFER_SIZE * 4 + 64);
            send(newsockfd, resp, resp_len, 0);
            free(resp);
        }
        close(newsockfd);
    }
}
