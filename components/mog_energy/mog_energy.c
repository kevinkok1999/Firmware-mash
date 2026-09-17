#include "mog_energy.h"

static bool budget_valid(const mog_energy_budget_t *budget) {
    return budget && budget->relay_budget_pct <= 100 &&
           budget->discovery_budget_pct <= 100 &&
           budget->multipath_budget_pct <= 100 &&
           budget->ip_background_budget_pct <= 100;
}

bool mog_energy_config_is_valid(const mog_energy_config_t *config) {
    if (!config) {
        return false;
    }

    const mog_energy_thresholds_t *t = &config->thresholds;
    if (t->survival_enter_mv == 0 ||
        !(t->survival_enter_mv < t->survival_exit_mv &&
          t->survival_exit_mv <= t->critical_enter_mv &&
          t->critical_enter_mv < t->critical_exit_mv &&
          t->critical_exit_mv <= t->conserve_enter_mv &&
          t->conserve_enter_mv < t->conserve_exit_mv) ||
        t->max_sample_age_ms == 0) {
        return false;
    }

    for (int i = 0; i < (int)MOG_ENERGY_STATE_COUNT; ++i) {
        if (!budget_valid(&config->budgets[i])) {
            return false;
        }
    }
    return true;
}

int mog_energy_manager_init(mog_energy_manager_t *manager,
                            const mog_energy_config_t *config,
                            mog_energy_state_t initial_state) {
    if (!manager || !config || initial_state < MOG_ENERGY_NORMAL ||
        initial_state >= MOG_ENERGY_STATE_COUNT) {
        return MOG_ENERGY_ERR_ARG;
    }
    if (!mog_energy_config_is_valid(config)) {
        return MOG_ENERGY_ERR_CONFIG;
    }

    manager->config = *config;
    manager->state = initial_state;
    manager->initialized = true;
    return MOG_ENERGY_OK;
}

static mog_energy_state_t classify_descending(const mog_energy_thresholds_t *t,
                                               uint32_t voltage_mv) {
    if (voltage_mv <= t->survival_enter_mv) {
        return MOG_ENERGY_SURVIVAL;
    }
    if (voltage_mv <= t->critical_enter_mv) {
        return MOG_ENERGY_CRITICAL;
    }
    if (voltage_mv <= t->conserve_enter_mv) {
        return MOG_ENERGY_CONSERVE;
    }
    return MOG_ENERGY_NORMAL;
}

static mog_energy_state_t next_state(const mog_energy_manager_t *manager,
                                     uint32_t voltage_mv) {
    const mog_energy_thresholds_t *t = &manager->config.thresholds;

    switch (manager->state) {
        case MOG_ENERGY_EXTERNAL_POWER:
            return classify_descending(t, voltage_mv);
        case MOG_ENERGY_NORMAL:
            return classify_descending(t, voltage_mv);
        case MOG_ENERGY_CONSERVE:
            if (voltage_mv <= t->survival_enter_mv) {
                return MOG_ENERGY_SURVIVAL;
            }
            if (voltage_mv <= t->critical_enter_mv) {
                return MOG_ENERGY_CRITICAL;
            }
            if (voltage_mv >= t->conserve_exit_mv) {
                return MOG_ENERGY_NORMAL;
            }
            return MOG_ENERGY_CONSERVE;
        case MOG_ENERGY_CRITICAL:
            if (voltage_mv <= t->survival_enter_mv) {
                return MOG_ENERGY_SURVIVAL;
            }
            if (voltage_mv >= t->conserve_exit_mv) {
                return MOG_ENERGY_NORMAL;
            }
            if (voltage_mv >= t->critical_exit_mv) {
                return MOG_ENERGY_CONSERVE;
            }
            return MOG_ENERGY_CRITICAL;
        case MOG_ENERGY_SURVIVAL:
            if (voltage_mv >= t->conserve_exit_mv) {
                return MOG_ENERGY_NORMAL;
            }
            if (voltage_mv >= t->critical_exit_mv) {
                return MOG_ENERGY_CONSERVE;
            }
            if (voltage_mv >= t->survival_exit_mv) {
                return MOG_ENERGY_CRITICAL;
            }
            return MOG_ENERGY_SURVIVAL;
        case MOG_ENERGY_STATE_COUNT:
            break;
    }
    return manager->state;
}

static void fill_snapshot(const mog_energy_manager_t *manager,
                          const mog_energy_sample_t *sample,
                          mog_energy_policy_snapshot_t *out) {
    out->state = manager->state;
    out->external_power = sample->external_power;
    out->source_flags = sample->source_flags;
    out->budget = manager->config.budgets[manager->state];
    out->confidence = sample->confidence;
    out->valid = true;
}

int mog_energy_manager_update(mog_energy_manager_t *manager,
                              const mog_energy_sample_t *sample,
                              mog_energy_policy_snapshot_t *out) {
    if (!manager || !sample || !out || !manager->initialized) {
        return MOG_ENERGY_ERR_ARG;
    }

    const mog_energy_thresholds_t *t = &manager->config.thresholds;
    if (!sample->available || sample->sample_age_ms > t->max_sample_age_ms ||
        sample->confidence < t->min_confidence) {
        return MOG_ENERGY_ERR_SAMPLE;
    }

    if (sample->external_power) {
        manager->state = MOG_ENERGY_EXTERNAL_POWER;
        fill_snapshot(manager, sample, out);
        return MOG_ENERGY_OK;
    }

    if (!sample->voltage_known || sample->voltage_mv == 0) {
        return MOG_ENERGY_ERR_SAMPLE;
    }

    manager->state = next_state(manager, sample->voltage_mv);
    fill_snapshot(manager, sample, out);
    return MOG_ENERGY_OK;
}
