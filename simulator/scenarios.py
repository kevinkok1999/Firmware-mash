"""Offline, hypothetical Wi-Fi security scenario descriptions.

This module does not control radios, transmit frames, or test live networks.
It is a teaching model, not a predictor of attack success.
"""

from dataclasses import dataclass


@dataclass(frozen=True)
class DefenseProfile:
    pmf_required: bool
    updated_router_firmware: bool
    network_monitoring: bool


def assess(scenario: str, profile: DefenseProfile) -> dict[str, str]:
    """Explain a conceptual scenario and relevant mitigations."""
    if scenario == "deauthentication":
        note = (
            "PMF is configured as required, which helps protect certain "
            "management frames. Other availability risks remain."
            if profile.pmf_required else
            "PMF is not required. Check whether your clients and router "
            "support requiring PMF (802.11w)."
        )
        return {
            "scenario": "Management-frame disruption (concept)",
            "observation": note,
            "next_step": "Validate WPA3/PMF configuration using router settings and supported clients.",
        }
    if scenario == "beacon":
        return {
            "scenario": "Misleading network advertisements (concept)",
            "observation": (
                "Monitoring is configured; investigate unexpected SSID/BSSID "
                "changes in legitimate network telemetry."
                if profile.network_monitoring else
                "There is no monitoring configured for unexpected network advertisements."
            ),
            "next_step": "Compare SSID and BSSID against your known access points.",
        }
    if scenario == "ap_availability":
        return {
            "scenario": "Access point availability degradation (concept)",
            "observation": (
                "Firmware is reported current; verify router health and logs."
                if profile.updated_router_firmware else
                "Firmware update status should be reviewed."
            ),
            "next_step": "Review router CPU/memory/connection telemetry if provided by the vendor.",
        }
    raise ValueError(f"Unknown offline scenario: {scenario}")


def main() -> None:
    profile = DefenseProfile(
        pmf_required=True,
        updated_router_firmware=True,
        network_monitoring=False,
    )
    print("BW16 PRO — offline hypothetical security scenarios\n")
    for scenario in ("deauthentication", "beacon", "ap_availability"):
        result = assess(scenario, profile)
        for key, value in result.items():
            print(f"{key}: {value}")
        print()


if __name__ == "__main__":
    main()
