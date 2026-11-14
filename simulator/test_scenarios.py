"""Unit tests for an offline educational model."""

import unittest
from scenarios import DefenseProfile, assess


class ScenarioTests(unittest.TestCase):
    def setUp(self):
        self.profile = DefenseProfile(
            pmf_required=True,
            updated_router_firmware=True,
            network_monitoring=True,
        )

    def test_pmf_note(self):
        self.assertIn("PMF", assess("deauthentication", self.profile)["observation"])

    def test_beacon_monitoring(self):
        self.assertIn("Monitoring", assess("beacon", self.profile)["observation"])

    def test_ap_availability(self):
        self.assertIn("Firmware", assess("ap_availability", self.profile)["observation"])

    def test_unrecognized_scenario(self):
        with self.assertRaises(ValueError):
            assess("unknown", self.profile)


if __name__ == "__main__":
    unittest.main()
