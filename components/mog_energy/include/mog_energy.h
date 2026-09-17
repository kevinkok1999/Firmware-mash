#ifndef MOG_ENERGY_H
#define MOG_ENERGY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOG_ENERGY_EXTERNAL_POWER = 0,
    MOG_ENERGY_NORMAL,
    MOG_ENERGY_CONSERVE,
    MOG_ENERGY_CRITICAL,
    MOG_ENERGY_SURVIVAL,
    MOG_ENERGY_STATE_COUNT,
} mog_energy_state_t;

typedef struct {
    bool available;
    bool external_power;
    bool voltage_known;
    uint32_t source_flags;
    uint32_t voltage_mv;
    int32_t current_ua;
    int32_t power_uw;
    uint32_t storage_voltage_mv;
    uint8_t confidence;
    uint32_t sample_age_ms;
} mog_energy_sample_t;

typedef struct {
    uint32_t survival_enter_mv;
    uint32_t survival_exit_mv;
    uint32_t critical_enter_mv;
    uint32_t critical_exit_mv;
    uint32_t conserve_enter_mv;
    uint32_t conserve_exit_mv;
    uint32_t max_sample_age_ms;
    uint8_t min_confidence;
} mog_energy_thresholds_t;

typedef struct {
    uint8_t relay_budget_pct;
    uint8_t discovery_budget_pct;
    uint8_t multipath_budget_pct;
    uint8_t ip_background_budget_pct;
    uint16_t energy_cost_bias;
} mog_energy_budget_t;

typedef struct {
    mog_energy_thresholds_t thresholds;
    mog_energy_budget_t budgets[MOG_ENERGY_STATE_COUNT];
} mog_energy_config_t;

typedef struct {
    mog_energy_state_t state;
    bool external_power;
    uint32_t source_flags;
    mog_energy_budget_t budget;
    uint8_t confidence;
    bool valid;
} mog_energy_policy_snapshot_t;

typedef struct {
    mog_energy_config_t config;
    mog_energy_state_t state;
    bool initialized;
} mog_energy_manager_t;

typedef enum {
    MOG_ENERGY_OK = 0,
    MOG_ENERGY_ERR_ARG = -1,
    MOG_ENERGY_ERR_CONFIG = -2,
    MOG_ENERGY_ERR_SAMPLE = -3,
} mog_energy_result_t;

/*
 * Thresholds are configuration/evidence inputs, never hard-coded board facts.
 * They must be populated from measured T-Deck battery/power behavior before a
 * target profile is promoted. The manager only owns policy/hysteresis.
 */
int mog_energy_manager_init(mog_energy_manager_t *manager,
                            const mog_energy_config_t *config,
                            mog_energy_state_t initial_state);

/* Invalid/stale/low-confidence samples do not mutate the current state. */
int mog_energy_manager_update(mog_energy_manager_t *manager,
                              const mog_energy_sample_t *sample,
                              mog_energy_policy_snapshot_t *out);

bool mog_energy_config_is_valid(const mog_energy_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* MOG_ENERGY_H */
