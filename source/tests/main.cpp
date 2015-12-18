/*
 * GraxRabble
 * example programs for libsodium.
 */

#include <sodium.h> /* library header */

/*
 * Utility functions shared by all the demo programs.
 */
#ifndef UTILS_H
#define UTILS_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sodium.h>

#define MAX_INPUT_LEN 4096

/*
 * print_hex() is a wrapper around sodium_bin2hex() which allocates
 * temporary memory then immediately prints the result followed by \n
 */
static void
print_hex(const void *bin, const size_t bin_len)
{
    char   *hex;
    size_t  hex_size;

    if (bin_len >= SIZE_MAX / 2) {
        abort();
    }
    hex_size = bin_len * 2 + 1;
    if ((hex = (char*)malloc(hex_size)) == NULL) {
        abort();
    }
    /* the library supplies a few utility functions like the one below */
    if (sodium_bin2hex(hex, hex_size, (const unsigned char*) bin, bin_len) == NULL) {
        abort();
    }
    puts(hex);
    free(hex);
}

/*
 * Display a prompt for input by user. It will save the input into a buffer
 * of a specific size with room for the null terminator while removing
 * trailing newline characters.
 */
static size_t
prompt_input(const char *prompt, char *input, const size_t max_input_len,
             int variable_length)
{
    char   input_tmp[MAX_INPUT_LEN + 1U];
    size_t actual_input_len;

    if (variable_length != 0) {
        printf("\nEnter %s (%zu bytes max) > ", prompt, max_input_len);
    } else {
        printf("\nEnter %s (%zu bytes) > ", prompt, max_input_len);
    }
    fflush(stdout);
    if (fgets(input_tmp, sizeof input_tmp, stdin) == NULL) {
        input_tmp[0] = '\0';
    }
    actual_input_len = strlen(input_tmp);

    /* trim \n */
    if (actual_input_len > 0 && input_tmp[actual_input_len - 1] == '\n') {
        input_tmp[actual_input_len - 1] = '\0';
        --actual_input_len;
    }

    if (actual_input_len > max_input_len) {
        printf("Warning: truncating input to %zu bytes\n\n", max_input_len);
        actual_input_len = max_input_len;
    } else if (actual_input_len < max_input_len && variable_length == 0) {
        printf("Warning: %zu bytes expected, %zu bytes given: padding with zeros\n\n",
               max_input_len, actual_input_len);
        memset(input, 0, max_input_len);
    } else {
        printf("Length: %zu bytes\n\n", actual_input_len);
    }

    memcpy(input, input_tmp, actual_input_len);
    if (variable_length == 0) {
        return max_input_len;
    } else {
        return actual_input_len;
    }
}

/*
 * Display whether the function was sucessful or failed.
 */
static void
print_verification(int ret)
{
    if (ret == 0)
        puts("Success!\n");
    else
        puts("Failure.\n");
}

static void
init(void)
{
    if (sodium_init() != 0) {
        abort();
    }
    printf("Using libsodium %s\n", sodium_version_string());
}

#endif /* UTILS_H */

/*
 * This operation computes an authentication tag for a message and a
 * secret key, and provides a way to verify that a given tag is valid
 * for a given message and a key.
 *
 * The function computing the tag deterministic: the same (message,
 * key) tuple will always produce the same output.
 *
 * However, even if the message is public, knowing the key is
 * required in order to be able to compute a valid tag. Therefore,
 * the key should remain confidential. The tag, however, can be
 * public.
 *
 * A typical use case is:
 *
 * - A prepares a message, add an authentication tag, sends it to B
 * - A doesn't store the message
 * - Later on, B sends the message and the authentication tag to A
 * - A uses the authentication tag to verify that it created this message.
 *
 * This operation does not encrypt the message. It only computes and
 * verifies an authentication tag.
 */
static int
auth(void)
{
    unsigned char key[crypto_auth_KEYBYTES];
    unsigned char mac[crypto_auth_BYTES];
    unsigned char message[MAX_INPUT_LEN];
    size_t        message_len;
    int           ret;

    puts("Example: crypto_auth\n");

    prompt_input("a key", (char*)key, sizeof key, 0);
    message_len = prompt_input("a message", (char*)message, sizeof message, 1);

    printf("Generating %s authentication...\n", crypto_auth_primitive());
    crypto_auth(mac, message, message_len, key);

    printf("Authentication tag: ");
    print_hex(mac, sizeof mac);

    puts("Verifying authentication tag...");
    ret = crypto_auth_verify(mac, message, message_len, key);
    print_verification(ret);

    sodium_memzero(key, sizeof key); /* wipe sensitive data */

    return ret;
}

int
main(void)
{
    init();

    return auth() != 0;
}