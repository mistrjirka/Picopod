import http.client
import importlib.util
import json
import pathlib
import os
import tempfile
import threading
import time
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
        self.cancelled = 0

    def current(self):
        return None

    def start(self, request):
        self.started.append(request)
        return type("Job", (), {"identifier": "http-job"})()

    def cancel(self):
        self.cancelled += 1
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

    def test_node_id_rejects_boolean_float_and_noncanonical_text(self):
        for value in (True, False, 1.0, "1.0", "-1", "+1", None):
            with self.assertRaises(module.UserInputError):
                module.validate_node_id(value)

    def test_boolean_fields_are_strict(self):
        self.assertTrue(module.parse_bool(True, "debug_echo"))
        self.assertFalse(module.parse_bool("off", "clean"))
        for value in (2, -1, "maybe", None, [], {}):
            with self.assertRaises(module.UserInputError):
                module.parse_bool(value, "clean")

    def test_unknown_request_fields_are_rejected(self):
        with self.assertRaisesRegex(module.UserInputError, "Unknown request field"):
            module.BuildRequest.from_json(
                {"device": "watch", "node_id": 1, "command": "rm -rf /"}
            )

    def test_suffix_exact_utf8_boundary_and_invalid_surrogate(self):
        exact = "ž" * 32
        self.assertEqual(len(exact.encode("utf-8")), module.MAX_SUFFIX_BYTES)
        self.assertEqual(module.validate_suffix(exact, True), exact)
        with self.assertRaises(module.UserInputError):
            module.validate_suffix(exact + "x", True)
        with self.assertRaises(module.UserInputError):
            module.validate_suffix("\ud800", True)

    def test_upload_port_rejects_traversal_whitespace_and_shell_metacharacters(self):
        bad = (
            "/dev/../tmp/ttyACM0",
            "/dev/ttyACM0 --upload-port /tmp/x",
            "/dev/ttyACM0;touch-x",
            "/dev/ttyACM0$(id)",
            "/dev/random",
        )
        for value in bad:
            with self.assertRaises(module.UserInputError):
                module.validate_upload_port(value)
        if os.name == "posix":
            self.assertEqual(
                module.validate_upload_port("/dev/serial/by-id/usb-Test_123-if00"),
                "/dev/serial/by-id/usb-Test_123-if00",
            )

    def test_log_offsets_continue_after_bounded_buffer_wrap(self):
        request = module.BuildRequest.from_json(
            {"device": "watch", "node_id": 7, "action": "build"}
        )
        job = module.BuildJob(request, mock.sentinel.workspace, "pio")
        for index in range(module.MAX_LOG_LINES + 7):
            job.log(f"line-{index}")
        snapshot = job.snapshot(0)
        self.assertTrue(snapshot["log_reset"])
        self.assertEqual(len(snapshot["logs"]), module.MAX_LOG_LINES)
        self.assertEqual(snapshot["logs"][0], "line-7")
        old_offset = snapshot["next_offset"]
        job.log("new-after-wrap")
        incremental = job.snapshot(old_offset)
        self.assertFalse(incremental["log_reset"])
        self.assertEqual(incremental["logs"], ["new-after-wrap"])

    def test_build_job_uses_argument_array_and_removes_generated_state(self):
        request = module.BuildRequest.from_json(
            {
                "device": "watch",
                "node_id": 44,
                "debug_echo": True,
                "suffix": " ; $(not-a-command)",
                "action": "build",
                "clean": False,
            }
        )
        with tempfile.TemporaryDirectory() as temporary:
            worktree = pathlib.Path(temporary)
            generated = worktree / "include" / "PicopodGeneratedConfig.h"
            generated.parent.mkdir(parents=True)
            generated.write_text("stale", encoding="utf-8")
            build_dir = worktree / ".pio" / "build" / request.device.environment
            build_dir.mkdir(parents=True)
            (build_dir / "firmware.bin").write_bytes(b"stale")

            workspace = mock.Mock()
            workspace.prepare.return_value = (worktree, "a" * 40)
            job = module.BuildJob(request, workspace, "/usr/bin/pio")
            calls = []

            def fake_stream(command, cwd, environment):
                calls.append((list(command), dict(environment)))
                self.assertFalse(generated.exists())
                generated.write_text("fresh", encoding="utf-8")
                (build_dir / "firmware.bin").write_bytes(b"new-firmware")
                return 0

            job._stream = fake_stream
            job._run_locked()
            self.assertEqual(job.state, "succeeded")
            self.assertFalse(generated.exists())
            self.assertEqual(calls[0][0], [
                "/usr/bin/pio", "run", "-e", "twatch-s3"
            ])
            self.assertNotIn(request.suffix, calls[0][0])
            self.assertEqual(
                calls[0][1]["PICOPOD_DEBUG_ECHO_SUFFIX"], request.suffix
            )
            self.assertEqual(pathlib.Path(job.artifact).read_bytes(), b"new-firmware")

    def test_stream_cancellation_terminates_child_process_group(self):
        request = module.BuildRequest.from_json(
            {"device": "watch", "node_id": 7, "action": "build"}
        )
        job = module.BuildJob(request, mock.sentinel.workspace, sys.executable)
        errors = []
        with tempfile.TemporaryDirectory() as temporary:
            def run_stream():
                try:
                    job._stream(
                        [
                            sys.executable,
                            "-u",
                            "-c",
                            "import time; print('started'); time.sleep(30)",
                        ],
                        pathlib.Path(temporary),
                        os.environ.copy(),
                    )
                except Exception as exc:
                    errors.append(exc)

            thread = threading.Thread(target=run_stream)
            thread.start()
            deadline = time.time() + 5
            while time.time() < deadline:
                with job._process_lock:
                    if job._process is not None:
                        break
                time.sleep(0.01)
            job.cancel()
            thread.join(timeout=5)
            self.assertFalse(thread.is_alive())
            self.assertEqual(len(errors), 1)
            self.assertIsInstance(errors[0], module.JobCancelled)

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

            first.state = "cancelled"
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

    def request(self, method, path, *, body=None, token=None, raw=False, content_type="application/json"):
        connection = http.client.HTTPConnection(self.host, self.port, timeout=3)
        headers = {}
        if token is not None:
            headers["X-Picopod-Token"] = token
        if body is not None:
            if not raw:
                body = json.dumps(body)
            headers["Content-Type"] = content_type
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


    def test_cancel_endpoint_is_authenticated_and_calls_manager(self):
        status, _headers, _payload = self.request("POST", "/api/cancel", body={})
        self.assertEqual(status, 403)
        status, _headers, payload = self.request(
            "POST", "/api/cancel", token=self.token, body={}
        )
        self.assertEqual(status, 200)
        self.assertEqual(json.loads(payload)["state"], "cancelling")
        self.assertEqual(self.manager.cancelled, 1)

    def test_start_rejects_wrong_content_type_and_malformed_json(self):
        status, _headers, payload = self.request(
            "POST",
            "/api/start",
            token=self.token,
            body="{}",
            raw=True,
            content_type="text/plain",
        )
        self.assertEqual(status, 400)
        self.assertIn("Content-Type", json.loads(payload)["error"])
        status, _headers, payload = self.request(
            "POST",
            "/api/start",
            token=self.token,
            body="{broken",
            raw=True,
        )
        self.assertEqual(status, 400)
        self.assertIn("Invalid request", json.loads(payload)["error"])



if __name__ == "__main__":
    unittest.main()
