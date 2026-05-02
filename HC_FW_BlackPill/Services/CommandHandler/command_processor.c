#include "command_processor.h"

#include "hc_jsonl_cmd.h"
#include "usb_vcp_drv.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COMMAND_PROCESSOR_RX_CHUNK_SIZE        (64u)
#define COMMAND_PROCESSOR_MESSAGE_BUFFER_SIZE  (256u)

typedef struct
{
    char Buffer[COMMAND_PROCESSOR_MESSAGE_BUFFER_SIZE];
    size_t Length;
    bool Collecting;
    bool InString;
    bool EscapeActive;
    bool Discarding;
    uint32_t BraceDepth;
} CommandProcessorContext_t;

static CommandProcessorContext_t s_CommandProcessor = {0};

static void command_processor_reset_state(void)
{
    s_CommandProcessor.Length = 0u;
    s_CommandProcessor.Buffer[0] = '\0';
    s_CommandProcessor.Collecting = false;
    s_CommandProcessor.InString = false;
    s_CommandProcessor.EscapeActive = false;
    s_CommandProcessor.Discarding = false;
    s_CommandProcessor.BraceDepth = 0u;
}

static void command_processor_begin_object(char ch)
{
    s_CommandProcessor.Buffer[0] = ch;
    s_CommandProcessor.Buffer[1] = '\0';
    s_CommandProcessor.Length = 1u;
    s_CommandProcessor.Collecting = true;
    s_CommandProcessor.InString = false;
    s_CommandProcessor.EscapeActive = false;
    s_CommandProcessor.Discarding = false;
    s_CommandProcessor.BraceDepth = 1u;
}

static void command_processor_enter_discard_mode(void)
{
    s_CommandProcessor.Length = 0u;
    s_CommandProcessor.Buffer[0] = '\0';
    s_CommandProcessor.Discarding = true;
}

static void command_processor_handle_byte(char ch)
{
    if (!s_CommandProcessor.Collecting)
    {
        if (ch == '{')
        {
            command_processor_begin_object(ch);
        }

        return;
    }

    if (!s_CommandProcessor.Discarding)
    {
        if (s_CommandProcessor.Length >= (COMMAND_PROCESSOR_MESSAGE_BUFFER_SIZE - 1u))
        {
            command_processor_enter_discard_mode();
        }
        else
        {
            s_CommandProcessor.Buffer[s_CommandProcessor.Length] = ch;
            s_CommandProcessor.Length++;
            s_CommandProcessor.Buffer[s_CommandProcessor.Length] = '\0';
        }
    }

    if (s_CommandProcessor.InString)
    {
        if (s_CommandProcessor.EscapeActive)
        {
            s_CommandProcessor.EscapeActive = false;
            return;
        }

        if (ch == '\\')
        {
            s_CommandProcessor.EscapeActive = true;
            return;
        }

        if (ch == '"')
        {
            s_CommandProcessor.InString = false;
        }

        return;
    }

    if (ch == '"')
    {
        s_CommandProcessor.InString = true;
        return;
    }

    if (ch == '{')
    {
        s_CommandProcessor.BraceDepth++;
        return;
    }

    if (ch == '}')
    {
        if (s_CommandProcessor.BraceDepth > 0u)
        {
            s_CommandProcessor.BraceDepth--;
        }

        if (s_CommandProcessor.BraceDepth == 0u)
        {
            if (!s_CommandProcessor.Discarding && (s_CommandProcessor.Length > 0u))
            {
                hc_jsonl_cmd_process_line(s_CommandProcessor.Buffer);
            }

            command_processor_reset_state();
        }
    }
}

void command_processor_init(void)
{
    command_processor_reset_state();
}

void command_processor_task(void)
{
    uint8_t rx_chunk[COMMAND_PROCESSOR_RX_CHUNK_SIZE];
    size_t bytes_read;
    size_t i;

    bytes_read = (size_t)usb_vcp_read(rx_chunk, sizeof(rx_chunk));
    if (bytes_read == 0u)
    {
        return;
    }

    for (i = 0u; i < bytes_read; ++i)
    {
        command_processor_handle_byte((char)rx_chunk[i]);
    }
}
