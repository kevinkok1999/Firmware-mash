#include "mog_dedup.h"

#include <string.h>

static mog_dedup_entry_t *find_mut(mog_dedup_t *dedup, mog_message_key_t key)
{
    size_t i;
    if (dedup == NULL) return NULL;
    for (i = 0; i < MOG_DEDUP_MAX_ENTRIES; ++i) {
        if (dedup->entries[i].in_use && mog_message_key_equal(dedup->entries[i].key, key))
            return &dedup->entries[i];
    }
    return NULL;
}

static mog_dedup_entry_t *alloc_entry(mog_dedup_t *dedup)
{
    size_t i;
    if (dedup == NULL || dedup->count >= MOG_DEDUP_MAX_ENTRIES) return NULL;
    for (i = 0; i < MOG_DEDUP_MAX_ENTRIES; ++i) {
        if (!dedup->entries[i].in_use) {
            memset(&dedup->entries[i], 0, sizeof(dedup->entries[i]));
            dedup->entries[i].in_use = true;
            dedup->count++;
            return &dedup->entries[i];
        }
    }
    return NULL;
}

void mog_dedup_init(mog_dedup_t *dedup)
{
    if (dedup != NULL) memset(dedup, 0, sizeof(*dedup));
}

const mog_dedup_entry_t *mog_dedup_find(const mog_dedup_t *dedup, mog_message_key_t key)
{
    size_t i;
    if (dedup == NULL || !mog_message_key_is_valid(key)) return NULL;
    for (i = 0; i < MOG_DEDUP_MAX_ENTRIES; ++i) {
        if (dedup->entries[i].in_use && mog_message_key_equal(dedup->entries[i].key, key))
            return &dedup->entries[i];
    }
    return NULL;
}

int mog_dedup_restore(mog_dedup_t *dedup, mog_message_key_t key,
                      bool delivered_to_chat, bool ack_required,
                      uint32_t received_at_ms)
{
    mog_dedup_entry_t *entry;
    if (dedup == NULL || !mog_message_key_is_valid(key)) return MOG_DEDUP_ERR_ARG;
    entry = find_mut(dedup, key);
    if (entry != NULL) {
        entry->durable = true;
        /* Durable restore is idempotent and may only strengthen chat truth. */
        entry->delivered_to_chat = entry->delivered_to_chat || delivered_to_chat;
        entry->ack_required = entry->ack_required || ack_required;
        return MOG_DEDUP_DUPLICATE;
    }
    entry = alloc_entry(dedup);
    if (entry == NULL) return MOG_DEDUP_ERR_FULL;
    entry->key = key;
    entry->received_at_ms = received_at_ms;
    entry->durable = true;
    entry->delivered_to_chat = delivered_to_chat;
    entry->ack_required = ack_required;
    return MOG_DEDUP_OK;
}

int mog_dedup_receive(mog_dedup_t *dedup, mog_message_key_t key,
                      uint32_t now_ms, bool *should_present,
                      bool *should_ack)
{
    mog_dedup_entry_t *entry;
    if (dedup == NULL || should_present == NULL || should_ack == NULL ||
        !mog_message_key_is_valid(key)) return MOG_DEDUP_ERR_ARG;

    *should_present = false;
    *should_ack = false;
    entry = find_mut(dedup, key);
    if (entry != NULL) {
        if (!entry->durable) {
            /* A duplicate racing the initial durable commit must not escape the
             * persist-before-present/ACK barrier. */
            return MOG_DEDUP_DUPLICATE;
        }
        *should_present = !entry->delivered_to_chat;
        entry->ack_required = true;
        *should_ack = true;
        return MOG_DEDUP_DUPLICATE;
    }

    entry = alloc_entry(dedup);
    if (entry == NULL) return MOG_DEDUP_ERR_FULL;
    entry->key = key;
    entry->received_at_ms = now_ms;
    entry->durable = false;
    entry->delivered_to_chat = false;
    entry->ack_required = true;
    /* No presentation and no ACK until MessageStore commit is confirmed. */
    return MOG_DEDUP_OK;
}

int mog_dedup_mark_durable(mog_dedup_t *dedup, mog_message_key_t key,
                           bool *should_present, bool *should_ack)
{
    mog_dedup_entry_t *entry;
    if (dedup == NULL || should_present == NULL || should_ack == NULL ||
        !mog_message_key_is_valid(key)) return MOG_DEDUP_ERR_ARG;
    entry = find_mut(dedup, key);
    if (entry == NULL) return MOG_DEDUP_ERR_NOT_FOUND;
    entry->durable = true;
    *should_present = !entry->delivered_to_chat;
    *should_ack = entry->ack_required;
    return MOG_DEDUP_OK;
}

int mog_dedup_mark_presented(mog_dedup_t *dedup, mog_message_key_t key)
{
    mog_dedup_entry_t *entry;
    if (dedup == NULL || !mog_message_key_is_valid(key)) return MOG_DEDUP_ERR_ARG;
    entry = find_mut(dedup, key);
    if (entry == NULL) return MOG_DEDUP_ERR_NOT_FOUND;
    if (!entry->durable) return MOG_DEDUP_ERR_NOT_DURABLE;
    entry->delivered_to_chat = true;
    return MOG_DEDUP_OK;
}

int mog_dedup_mark_ack_sent(mog_dedup_t *dedup, mog_message_key_t key)
{
    mog_dedup_entry_t *entry;
    if (dedup == NULL || !mog_message_key_is_valid(key)) return MOG_DEDUP_ERR_ARG;
    entry = find_mut(dedup, key);
    if (entry == NULL) return MOG_DEDUP_ERR_NOT_FOUND;
    if (!entry->durable) return MOG_DEDUP_ERR_NOT_DURABLE;
    entry->ack_required = false;
    return MOG_DEDUP_OK;
}
