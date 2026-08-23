import http.client
import importlib.util
import json
import pathlib
import os
import tempfile
import threading
import unittest
from unittest import mock
import sys

MODULE_PATH = pathlib.Path(__file__).resolve().parents[1] / "picopod_flash_ui.py"
spec = importlib.util.spec_from_file_location("picopod_flash_ui", MODULE_PATH)
module = importlib.util.module_from_spec(spec)
assert spec.loader
sys.modules[spec.name] = module
spec.loader.exec_module(module)


class FakeHttpManager:
    pio = "/bin/false"

    def __init__(self):
        self.started = []

    def current(self):
        return None

    def start(self, request):
        self.started.append(request)
        return type("Job", (), {"identifier": "http-job"})()

class ValidationTests(unittest.TestCase):
    def test_node_id_bounds(self):
        self.assertEqual(module.validate_node_id("1"), 1)
        self.assertEqual(module.validate_node_id(65535), 65535)
        for value in (0, 65536, "no"):
            with self.assertRaises(module.UserInputError):
                module.validate_node_id(value)

    def test_echo_suffix_is_bounded_by_utf8_bytes(self):
        self.assertEqual(module.validate_suffix(" [echo]", True), " [echo]")
        with self.assertRaises(module.UserInputError):
            module.validate_suffix("", True)
        with self.assertRaises(module.UserInputError):
            module.validate_suffix("ž" * 33, True)
        self.assertEqual(module.validate_suffix("", False), "")

    def test_request_builds_safe_environment(self):
        request = module.BuildRequest.from_json(
            {
                "device": "watch",
                "node_id": "44",
                "debug_echo": True,
                "suffix": " [test]",
                "action": "flash",
                "upload_port": "",
                "clean": True,
            }
        )
        environment = request.environment()
        self.assertEqual(request.device.branch, "espwatch")
        self.assertEqual(environment["PICOPOD_NODE_ID"], "44")
        self.assertEqual(environment["PICOPOD_DEBUG_ECHO"], "1")
        self.assertEqual(environment["PICOPOD_DEBUG_ECHO_SUFFIX"], " [test]")

    def test_unknown_device_and_action_are_rejected(self):
        with self.assertRaises(module.UserInputError):
            module.BuildRequest.from_json({"device": "unknown", "node_id": 1})
        with self.assertRaises(module.UserInputError):
            module.BuildRequest.from_json(
                {"device": "watch", "node_id": 1, "action": "erase"}
            )

    def test_upload_port_validation_rejects_non_device_and_control_characters(self):
        self.assertEqual(module.validate_upload_port("auto"), "")
        self.assertEqual(module.validate_upload_port(""), "")
        if os.name == "posix":
            self.assertEqual(
                module.validate_upload_port("/dev/ttyACM0"), "/dev/ttyACM0"
            )
            for value in ("ttyACM0", "/tmp/fake-port"):
                with self.assertRaises(module.UserInputError):
                    module.validate_upload_port(value)
        for value in ("/dev/ttyACM0\n--evil", "/dev/ttyACM0\x00x"):
            with self.assertRaises(module.UserInputError):
                module.validate_upload_port(value)

    def test_job_manager_serializes_generated_configuration_and_flash_access(self):
        request = module.BuildRequest.from_json(
            {
                "device": "watch",
                "node_id": 44,
                "debug_echo": True,
                "suffix": " [test]",
                "action": "build",
                "upload_port": "",
                "clean": False,
            }
        )
        manager = module.JobManager(mock.sentinel.workspace, "pio")
        with mock.patch.object(module.BuildJob, "start", autospec=True) as start:
            first = manager.start(request)
            start.assert_called_once_with(first)
            self.assertEqual(first.state, "queued")
            with self.assertRaisesRegex(
                module.UserInputError,
                "Another build or flash operation is still running",
            ):
                manager.start(request)

            first.state = "succeeded"
            second = manager.start(request)
            self.assertIs(manager.current(), second)
            self.assertIsNot(first, second)
            self.assertEqual(start.call_count, 2)

    def test_workspace_lock_rejects_a_second_flasher_process(self):
        if module.fcntl is None:
            self.skipTest("POSIX flock is unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            cache = pathlib.Path(temporary)
            first = module.WorkspaceManager("unused", cache)
            second = module.WorkspaceManager("unused", cache)
            with first.operation_lock():
                with self.assertRaisesRegex(
                    module.UserInputError,
                    "Another Picopod flasher process is using this build cache",
                ):
                    with second.operation_lock():
                        self.fail("second process lock unexpectedly succeeded")
            with second.operation_lock():
                pass

    def test_device_mapping_points_to_production_branches(self):
        self.assertEqual(module.DEVICES["watch"].branch, "espwatch")
        self.assertEqual(
            module.DEVICES["heltec"].branch, "heltec-stick-lite-v3"
        )
        self.assertEqual(
            module.DEVICES["rp2040"].branch, "refactor-to-radiolib"
        )


class HttpApiTests(unittest.TestCase):
    def setUp(self):
        self.manager = FakeHttpManager()
        self.token = "unit-test-token"
        self.server = module.create_server(
            module.App(self.manager, self.token), "127.0.0.1", 0
        )
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.host, self.port = self.server.server_address[:2]

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)

    def request(self, method, path, *, body=None, token=None):
        connection = http.client.HTTPConnection(self.host, self.port, timeout=3)
        headers = {}
        if token is not None:
            headers["X-Picopod-Token"] = token
        if body is not None:
            body = json.dumps(body)
            headers["Content-Type"] = "application/json"
        connection.request(method, path, body=body, headers=headers)
        response = connection.getresponse()
        payload = response.read()
        headers_out = dict(response.getheaders())
        connection.close()
        return response.status, headers_out, payload

    def test_page_is_local_hardened_and_contains_device_choices(self):
        status, headers, payload = self.request("GET", "/")
        self.assertEqual(status, 200)
        text = payload.decode("utf-8")
        self.assertIn("LilyGo T-Watch S3", text)
        self.assertIn("Heltec Wireless Stick Lite V3", text)
        self.assertIn("Custom RP2040 Picopod", text)
        self.assertIn(self.token, text)
        self.assertIn("default-src 'none'", headers["Content-Security-Policy"])
        self.assertEqual(headers["X-Content-Type-Options"], "nosniff")

    def test_api_requires_token_and_validates_request(self):
        status, _headers, _payload = self.request("GET", "/api/status")
        self.assertEqual(status, 403)
        status, _headers, payload = self.request(
            "GET", "/api/status", token=self.token
        )
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(payload)["state"], "idle")

        status, _headers, payload = self.request(
            "POST",
            "/api/start",
            token=self.token,
            body={"device": "unknown", "node_id": 1},
        )
        self.assertEqual(status, 400)
        self.assertIn("Unknown device", json.loads(payload)["error"])

        status, _headers, payload = self.request(
            "POST",
            "/api/start",
            token=self.token,
            body={
                "device": "heltec",
                "node_id": 42,
                "action": "build",
                "debug_echo": True,
                "suffix": " [test]",
            },
        )
        self.assertEqual(status, 202)
        self.assertEqual(json.loads(payload)["job_id"], "http-job")
        self.assertEqual(self.manager.started[-1].node_id, 42)


if __name__ == "__main__":
    unittest.main()
