#include "mog_energy.h"

#include <assert.h>
#include <stdio.h>

static mog_energy_config_t config(void) {
    mog_energy_config_t cfg = {0};
    cfg.thresholds = (mog_energy_thresholds_t){
        .survival_enter_mv = 3200,
        .survival_exit_mv = 3300,
        .critical_enter_mv = 3400,
        .critical_exit_mv = 3500,
        .conserve_enter_mv = 3600,
        .conserve_exit_mv = 3700,
        .max_sample_age_ms = 5000,
        .min_confidence = 50,
    };

    cfg.budgets[MOG_ENERGY_EXTERNAL_POWER] = (mog_energy_budget_t){100, 100, 100, 100, 0};
    cfg.budgets[MOG_ENERGY_NORMAL] = (mog_energy_budget_t){100, 100, 100, 100, 10};
    cfg.budgets[MOG_ENERGY_CONSERVE] = (mog_energy_budget_t){60, 60, 50, 40, 30};
    cfg.budgets[MOG_ENERGY_CRITICAL] = (mog_energy_budget_t){25, 30, 20, 10, 60};
    cfg.budgets[MOG_ENERGY_SURVIVAL] = (mog_energy_budget_t){5, 10, 0, 0, 100};
    return cfg;
}

static mog_energy_sample_t sample(uint32_t voltage_mv) {
    return (mog_energy_sample_t){
        .available = true,
        .external_power = false,
        .voltage_known = true,
        .voltage_mv = voltage_mv,
        .confidence = 100,
        .sample_age_ms = 0,
    };
}

static void test_hysteresis_prevents_flapping(void) {
    mog_energy_config_t cfg = config();
    mog_energy_manager_t manager;
    mog_energy_policy_snapshot_t out;
    assert(mog_energy_manager_init(&manager, &cfg, MOG_ENERGY_NORMAL) == MOG_ENERGY_OK);

    mog_energy_sample_t s = sample(3550);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_CONSERVE);

    s = sample(3650);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_CONSERVE);

    s = sample(3750);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_NORMAL);
}

static void test_deep_drop_and_recovery(void) {
    mog_energy_config_t cfg = config();
    mog_energy_manager_t manager;
    mog_energy_policy_snapshot_t out;
    assert(mog_energy_manager_init(&manager, &cfg, MOG_ENERGY_NORMAL) == MOG_ENERGY_OK);

    mog_energy_sample_t s = sample(3100);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_SURVIVAL);
    assert(out.budget.ip_background_budget_pct == 0);

    s = sample(3250);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_SURVIVAL);

    s = sample(3350);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_CRITICAL);

    s = sample(3550);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_CONSERVE);
}

static void test_external_power_overrides_battery_policy(void) {
    mog_energy_config_t cfg = config();
    mog_energy_manager_t manager;
    mog_energy_policy_snapshot_t out;
    assert(mog_energy_manager_init(&manager, &cfg, MOG_ENERGY_CRITICAL) == MOG_ENERGY_OK);

    mog_energy_sample_t s = sample(3300);
    s.external_power = true;
    s.voltage_known = false;
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_EXTERNAL_POWER);
    assert(out.external_power);

    s = sample(3550);
    assert(mog_energy_manager_update(&manager, &s, &out) == MOG_ENERGY_OK);
    assert(out.state == MOG_ENERGY_CONSERVE);
}

static void test_bad_samples_do_not_mutate_state(void) {
    mog_energy_config_t cfg = config();
    mog_energy_manager_t manager;
    mog_energy_policy_snapshot_t out = {0};
    assert(mog_energy_manager_init(&manager, &cfg, MOG_ENERGY_NORMAL) == MOG_ENERGY_OK);

    mog_energy_sample_t stale = sample(3500);
    stale.sample_age_ms = cfg.thresholds.max_sample_age_ms + 1;
    assert(mog_energy_manager_update(&manager, &stale, &out) == MOG_ENERGY_ERR_SAMPLE);
    assert(manager.state == MOG_ENERGY_NORMAL);

    mog_energy_sample_t weak = sample(3500);
    weak.confidence = cfg.thresholds.min_confidence - 1;
    assert(mog_energy_manager_update(&manager, &weak, &out) == MOG_ENERGY_ERR_SAMPLE);
    assert(manager.state == MOG_ENERGY_NORMAL);

    mog_energy_sample_t unknown = sample(3500);
    unknown.voltage_known = false;
    assert(mog_energy_manager_update(&manager, &unknown, &out) == MOG_ENERGY_ERR_SAMPLE);
    assert(manager.state == MOG_ENERGY_NORMAL);
}

static void test_invalid_thresholds_fail_closed(void) {
    mog_energy_config_t cfg = config();
    cfg.thresholds.critical_exit_mv = cfg.thresholds.critical_enter_mv;
    assert(!mog_energy_config_is_valid(&cfg));

    mog_energy_manager_t manager;
    assert(mog_energy_manager_init(&manager, &cfg, MOG_ENERGY_NORMAL) == MOG_ENERGY_ERR_CONFIG);
}

int main(void) {
    test_hysteresis_prevents_flapping();
    test_deep_drop_and_recovery();
    test_external_power_overrides_battery_policy();
    test_bad_samples_do_not_mutate_state();
    test_invalid_thresholds_fail_closed();
    puts("test_energy: PASS");
    return 0;
}
