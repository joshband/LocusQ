#!/usr/bin/env python3
"""Structured Appium/mac2 regression lane for LocusQ standalone UI on macOS."""

from __future__ import annotations

import argparse
import io
import json
import os
import plistlib
import signal
import subprocess
import sys
import time
import traceback
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from PIL import Image, ImageChops, ImageStat


@dataclass(frozen=True)
class UITestCase:
    test_id: str
    kind: str
    click_x: int
    click_y: int
    roi_x: int
    roi_y: int
    roi_w: int
    roi_h: int
    threshold: float
    text_value: str = ""


DEFAULT_TESTS = [
    UITestCase(
        test_id="UI-01-tab-renderer",
        kind="click",
        click_x=275,
        click_y=58,
        roi_x=150,
        roi_y=40,
        roi_w=340,
        roi_h=84,
        threshold=0.10,
    ),
    UITestCase(
        test_id="UI-01-tab-emitter",
        kind="click",
        click_x=180,
        click_y=58,
        roi_x=150,
        roi_y=40,
        roi_w=340,
        roi_h=84,
        threshold=0.10,
    ),
    UITestCase(
        test_id="UI-02-quality-badge",
        kind="click",
        click_x=1185,
        click_y=58,
        roi_x=1040,
        roi_y=30,
        roi_w=220,
        roi_h=80,
        threshold=0.015,
    ),
    UITestCase(
        test_id="UI-03-toggle-size",
        kind="click",
        click_x=1228,
        click_y=394,
        roi_x=1148,
        roi_y=344,
        roi_w=132,
        roi_h=92,
        threshold=0.01,
    ),
    UITestCase(
        test_id="UI-04-pos-mode-dd",
        kind="click",
        click_x=1212,
        click_y=217,
        roi_x=1020,
        roi_y=168,
        roi_w=258,
        roi_h=92,
        threshold=0.01,
    ),
    UITestCase(
        test_id="UI-05-emit-label",
        kind="text",
        click_x=1140,
        click_y=139,
        roi_x=980,
        roi_y=98,
        roi_w=300,
        roi_h=84,
        threshold=0.01,
        text_value="AutoUITest",
    ),
]


def utc_timestamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")


def iso_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run Appium/mac2 standalone UI regression checks for LocusQ."
    )
    parser.add_argument("--app-path", default="", help="Path to LocusQ.app.")
    parser.add_argument("--bundle-id", default="", help="CFBundleIdentifier for app.")
    parser.add_argument(
        "--server-url", default="http://127.0.0.1:4723", help="Appium server URL."
    )
    parser.add_argument(
        "--output-root",
        default="TestEvidence",
        help="Root directory for run artifacts.",
    )
    parser.add_argument("--window-x", type=int, default=120)
    parser.add_argument("--window-y", type=int, default=80)
    parser.add_argument("--window-w", type=int, default=1280)
    parser.add_argument("--window-h", type=int, default=820)
    parser.add_argument(
        "--startup-delay",
        type=float,
        default=1.1,
        help="Seconds to wait after creating a session.",
    )
    parser.add_argument(
        "--interaction-delay",
        type=float,
        default=0.40,
        help="Seconds to wait after each interaction.",
    )
    parser.add_argument(
        "--content-x-offset",
        type=int,
        default=0,
        help="Extra X offset from window origin to content origin (points).",
    )
    parser.add_argument(
        "--content-y-offset",
        type=int,
        default=None,
        help="Extra Y offset from window origin to content origin (points). Auto when unset.",
    )
    parser.add_argument(
        "--skip-ui05",
        action="store_true",
        help="Skip text-input check (UI-05).",
    )
    parser.add_argument(
        "--step-timeout-seconds",
        type=float,
        default=10.0,
        help="Per-Appium-call timeout in seconds (fail-fast guard).",
    )
    parser.add_argument(
        "--max-run-seconds",
        type=float,
        default=180.0,
        help="Overall max wall time for the run in seconds (fail-fast guard).",
    )
    return parser.parse_args()


def default_app_candidates(repo_root: Path) -> list[Path]:
    return [
        repo_root / "build_local/LocusQ_artefacts/Release/Standalone/LocusQ.app",
        repo_root / "build/LocusQ_artefacts/Standalone/LocusQ.app",
        repo_root / "build_ship_universal/LocusQ_artefacts/Release/Standalone/LocusQ.app",
        Path("/Applications/LocusQ.app"),
    ]


def resolve_app_path(arg_path: str, repo_root: Path) -> Path:
    if arg_path:
        candidate = Path(arg_path).expanduser().resolve()
        if candidate.is_dir():
            return candidate
        raise FileNotFoundError(f"App path not found: {candidate}")
    for candidate in default_app_candidates(repo_root):
        if candidate.is_dir():
            return candidate.resolve()
    raise FileNotFoundError("Unable to locate LocusQ.app in default paths.")


def infer_bundle_id(app_path: Path) -> str:
    plist_path = app_path / "Contents/Info.plist"
    if not plist_path.is_file():
        return "com.apc.LocusQ"
    with plist_path.open("rb") as handle:
        plist = plistlib.load(handle)
    return str(plist.get("CFBundleIdentifier") or "com.apc.LocusQ")


def open_app(app_path: Path) -> None:
    subprocess.run(["open", "-na", str(app_path)], check=False)


def run_osascript(script: str) -> tuple[int, str, str]:
    proc = subprocess.run(
        ["osascript", "-"],
        input=script,
        text=True,
        capture_output=True,
        check=False,
    )
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def set_window_geometry_macos(
    app_name: str,
    x: int,
    y: int,
    w: int,
    h: int,
) -> tuple[bool, str]:
    script = f"""
tell application "{app_name}" to activate
delay 0.35
tell application "System Events"
  tell process "{app_name}"
    set frontmost to true
    if (count of windows) = 0 then error "No front window for {app_name}"
    set position of front window to {{{x}, {y}}}
    set size of front window to {{{w}, {h}}}
  end tell
end tell
"""
    rc, out, err = run_osascript(script)
    if rc != 0:
        return False, err or out or "unknown osascript failure"
    return True, out or "ok"


def get_window_geometry_macos(app_name: str) -> tuple[dict[str, int] | None, str]:
    script = f"""
tell application "System Events"
  tell process "{app_name}"
    if (count of windows) = 0 then error "No front window for {app_name}"
    set p to value of attribute "AXPosition" of front window
    set s to value of attribute "AXSize" of front window
    set xPos to (item 1 of p) as integer
    set yPos to (item 2 of p) as integer
    set w to (item 1 of s) as integer
    set h to (item 2 of s) as integer
    return (xPos as string) & "," & (yPos as string) & "," & (w as string) & "," & (h as string)
  end tell
end tell
"""
    rc, out, err = run_osascript(script)
    if rc != 0:
        return None, err or out or "unknown osascript failure"
    raw = out.replace(" ", "")
    parts = raw.split(",")
    if len(parts) != 4:
        return None, f"unexpected geometry output: {out!r}"
    try:
        x_pos, y_pos, w_pos, h_pos = (int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3]))
    except ValueError:
        return None, f"non-integer geometry output: {out!r}"
    return {"x": x_pos, "y": y_pos, "w": w_pos, "h": h_pos}, ""


def detect_content_y_offset(image: Image.Image) -> int:
    width, height = image.size
    limit = min(height, 560)
    if limit <= 0:
        return 0
    lum: list[float] = []

    for y in range(limit):
        row = image.crop((0, y, width, y + 1))
        r, g, b = ImageStat.Stat(row).mean
        lum.append(0.2126 * r + 0.7152 * g + 0.0722 * b)

    band: tuple[int, int] | None = None
    y = 20
    while y < limit:
        if lum[y] > 170:
            start = y
            while y < limit and lum[y] > 170:
                y += 1
            end = y - 1
            if end - start + 1 >= 8:
                band = (start, end)
                break
        else:
            y += 1

    if band is not None:
        _, end = band
        for yy in range(end + 1, min(limit, end + 220)):
            if lum[yy] < 90:
                return yy

    if limit < 8:
        return 0
    for yy in range(20, max(20, limit - 7)):
        if yy + 7 >= len(lum):
            break
        if all(lum[yy + k] < 90 for k in range(8)):
            return yy

    return 0


def screenshot_image(driver: Any, out_path: Path) -> Image.Image:
    png_bytes = driver.get_screenshot_as_png()
    out_path.write_bytes(png_bytes)
    with Image.open(io.BytesIO(png_bytes)) as img:
        return img.convert("RGB")


def _timeout_handler(signum: int, frame: Any) -> None:
    raise TimeoutError("operation timed out")


def call_with_timeout(
    timeout_seconds: float,
    label: str,
    func: Any,
    *args: Any,
    **kwargs: Any,
) -> Any:
    if timeout_seconds <= 0:
        return func(*args, **kwargs)
    previous = signal.getsignal(signal.SIGALRM)
    signal.signal(signal.SIGALRM, _timeout_handler)
    signal.setitimer(signal.ITIMER_REAL, timeout_seconds)
    try:
        return func(*args, **kwargs)
    except TimeoutError as exc:
        raise TimeoutError(f"{label} timed out after {timeout_seconds:.1f}s") from exc
    finally:
        signal.setitimer(signal.ITIMER_REAL, 0)
        signal.signal(signal.SIGALRM, previous)


def ensure_run_budget(start_monotonic: float, max_run_seconds: float, label: str) -> None:
    if max_run_seconds <= 0:
        return
    elapsed = time.monotonic() - start_monotonic
    if elapsed > max_run_seconds:
        raise TimeoutError(
            f"overall run timeout exceeded at {label}: {elapsed:.1f}s > {max_run_seconds:.1f}s"
        )


def clamp_box(box: tuple[int, int, int, int], width: int, height: int) -> tuple[int, int, int, int]:
    x0, y0, x1, y1 = box
    x0 = max(0, min(width - 1, x0))
    y0 = max(0, min(height - 1, y0))
    x1 = max(x0 + 1, min(width, x1))
    y1 = max(y0 + 1, min(height, y1))
    return x0, y0, x1, y1


def diff_score(before: Image.Image, after: Image.Image, box: tuple[int, int, int, int]) -> float:
    b = before.crop(box)
    a = after.crop(box)
    diff = ImageChops.difference(b, a)
    stat = ImageStat.Stat(diff)
    return float(sum(stat.mean) / len(stat.mean))


def safe_message(exc: Exception) -> str:
    text = str(exc).strip()
    return text or exc.__class__.__name__


def click_rel(
    driver: Any,
    origin_x: int,
    origin_y: int,
    content_x_offset: int,
    content_y_offset: int,
    rel_x: int,
    rel_y: int,
    step_timeout_seconds: float,
) -> dict[str, Any]:
    warnings: list[str] = []
    abs_x = int(origin_x + content_x_offset + rel_x)
    abs_y = int(origin_y + content_y_offset + rel_y)

    try:
        call_with_timeout(
            step_timeout_seconds,
            "macos: click",
            driver.execute_script,
            "macos: click",
            {"x": abs_x, "y": abs_y},
        )
        return {"method": "macos: click", "abs_x": abs_x, "abs_y": abs_y, "warnings": warnings}
    except Exception as exc:  # pragma: no cover - command availability varies by driver version
        warnings.append(f"macos: click failed: {safe_message(exc)}")

    from selenium.webdriver.common.actions import interaction
    from selenium.webdriver.common.actions.action_builder import ActionBuilder
    from selenium.webdriver.common.actions.pointer_input import PointerInput

    try:
        vx = int(content_x_offset + rel_x)
        vy = int(content_y_offset + rel_y)
        action = ActionBuilder(driver, mouse=PointerInput(interaction.POINTER_MOUSE, "mouse"))
        action.pointer_action.move_to_location(vx, vy)
        action.pointer_action.click()
        call_with_timeout(step_timeout_seconds, "w3c pointer click", action.perform)
        return {"method": "w3c:pointer", "viewport_x": vx, "viewport_y": vy, "warnings": warnings}
    except Exception as exc:
        warnings.append(f"w3c pointer click failed: {safe_message(exc)}")
        raise RuntimeError("; ".join(warnings)) from exc


def type_replace(driver: Any, text: str, step_timeout_seconds: float) -> dict[str, Any]:
    warnings: list[str] = []

    try:
        for ch in text:
            call_with_timeout(
                step_timeout_seconds,
                "macos: keys(char)",
                driver.execute_script,
                "macos: keys",
                {"keys": [ch]},
            )
        return {"method": "macos: keys(chars)", "warnings": warnings}
    except Exception as exc:  # pragma: no cover - command availability varies by driver version
        warnings.append(f"macos: keys(chars) failed: {safe_message(exc)}")

    from selenium.webdriver.common.action_chains import ActionChains

    try:
        actions = ActionChains(driver).send_keys(text)
        call_with_timeout(step_timeout_seconds, "w3c key entry", actions.perform)
        return {"method": "w3c:keys(text)", "warnings": warnings}
    except Exception as exc:
        warnings.append(f"w3c key entry failed: {safe_message(exc)}")
        raise RuntimeError("; ".join(warnings)) from exc


def build_driver(server_url: str, capabilities: dict[str, Any]) -> Any:
    from appium import webdriver
    from appium.options.common import AppiumOptions

    options = AppiumOptions()
    options.load_capabilities(capabilities)
    return webdriver.Remote(server_url, options=options)


def run() -> int:
    args = parse_args()
    start_monotonic = time.monotonic()
    repo_root = Path(__file__).resolve().parents[2]

    app_path = resolve_app_path(args.app_path, repo_root)
    app_name = app_path.stem
    bundle_id = args.bundle_id or infer_bundle_id(app_path)
    output_root = Path(args.output_root)
    out_dir = output_root / f"appium_ui_regression_{utc_timestamp()}"
    out_dir.mkdir(parents=True, exist_ok=True)

    summary_path = out_dir / "summary.tsv"
    report_path = out_dir / "report.json"

    summary_lines = [
        "test_id\tresult\tdiff_score\tthreshold\troi_box_px\tartifacts\tinteraction_method\tinteraction_warnings"
    ]
    report: dict[str, Any] = {
        "started_at": iso_now(),
        "environment": {
            "platform": sys.platform,
            "python": sys.version.split()[0],
            "cwd": os.getcwd(),
            "app_path": str(app_path),
            "app_name": app_name,
            "bundle_id": bundle_id,
            "server_url": args.server_url,
        },
        "capabilities": {
            "platformName": "mac",
            "appium:automationName": "mac2",
            "appium:bundleId": bundle_id,
            "appium:appPath": str(app_path),
            "appium:newCommandTimeout": 240,
        },
        "window_rect": {},
        "window_rect_osascript": {},
        "window_geometry_source": "",
        "click_origin_points": {},
        "screenshot_scope": "",
        "scale_factors": {},
        "content_offset_points": {},
        "errors": [],
        "tests": [],
        "rawLogs": [],
    }

    open_app(app_path)

    driver = None
    exit_code = 1
    try:
        ensure_run_budget(start_monotonic, args.max_run_seconds, "session-create")
        driver = call_with_timeout(
            args.step_timeout_seconds, "create Appium session", build_driver, args.server_url, report["capabilities"]
        )
        try:
            ensure_run_budget(start_monotonic, args.max_run_seconds, "activate-app")
            call_with_timeout(args.step_timeout_seconds, "activate app", driver.activate_app, bundle_id)
        except Exception:
            pass

        ensure_run_budget(start_monotonic, args.max_run_seconds, "window-geometry")
        os_window, os_window_err = get_window_geometry_macos(app_name)
        if os_window is None:
            ok, msg = set_window_geometry_macos(app_name, args.window_x, args.window_y, args.window_w, args.window_h)
            if not ok:
                report["rawLogs"].append(f"osascript set_window_geometry failed: {msg}")
            if args.startup_delay > 0:
                time.sleep(min(args.startup_delay, 0.6))
            os_window, os_window_err = get_window_geometry_macos(app_name)
            if os_window is None and os_window_err:
                report["rawLogs"].append(f"osascript get_window_geometry failed: {os_window_err}")

        try:
            ensure_run_budget(start_monotonic, args.max_run_seconds, "set-window-rect")
            call_with_timeout(
                args.step_timeout_seconds,
                "set window rect",
                driver.set_window_rect,
                x=args.window_x,
                y=args.window_y,
                width=args.window_w,
                height=args.window_h,
            )
        except Exception as exc:
            report["rawLogs"].append(f"set_window_rect failed: {safe_message(exc)}")

        if args.startup_delay > 0:
            time.sleep(args.startup_delay)

        ensure_run_budget(start_monotonic, args.max_run_seconds, "get-window-rect")
        window_rect = call_with_timeout(args.step_timeout_seconds, "get window rect", driver.get_window_rect)
        report["window_rect"] = {
            "x": int(window_rect["x"]),
            "y": int(window_rect["y"]),
            "width": int(window_rect["width"]),
            "height": int(window_rect["height"]),
        }
        if os_window is not None:
            report["window_rect_osascript"] = os_window
            report["window_geometry_source"] = "osascript"
        else:
            report["window_geometry_source"] = "appium_window_rect"

        click_origin_x = int(os_window["x"]) if os_window is not None else int(report["window_rect"]["x"])
        click_origin_y = int(os_window["y"]) if os_window is not None else int(report["window_rect"]["y"])
        report["click_origin_points"] = {"x": click_origin_x, "y": click_origin_y}

        bootstrap_path = out_dir / "_bootstrap_window.png"
        ensure_run_budget(start_monotonic, args.max_run_seconds, "bootstrap-screenshot")
        bootstrap_img = call_with_timeout(
            args.step_timeout_seconds, "bootstrap screenshot", screenshot_image, driver, bootstrap_path
        )
        scale_x = bootstrap_img.width / max(report["window_rect"]["width"], 1)
        scale_y = bootstrap_img.height / max(report["window_rect"]["height"], 1)
        report["scale_factors"] = {"x": scale_x, "y": scale_y}

        screenshot_scope = "window_like"
        if os_window is not None:
            wr_w = max(report["window_rect"]["width"], 1)
            wr_h = max(report["window_rect"]["height"], 1)
            if os_window["w"] < int(wr_w * 0.95) or os_window["h"] < int(wr_h * 0.95):
                screenshot_scope = "desktop"
        report["screenshot_scope"] = screenshot_scope

        content_y_offset = (
            int(args.content_y_offset)
            if args.content_y_offset is not None
            else int(round(detect_content_y_offset(bootstrap_img) / scale_y))
        )
        content_x_offset = int(args.content_x_offset)
        report["content_offset_points"] = {"x": content_x_offset, "y": content_y_offset}

        cases = [c for c in DEFAULT_TESTS if not (args.skip_ui05 and c.test_id == "UI-05-emit-label")]

        # Precondition tab state so renderer/emitter checks are deterministic.
        try:
            ensure_run_budget(start_monotonic, args.max_run_seconds, "prep-calibrate")
            prep = click_rel(
                driver,
                click_origin_x,
                click_origin_y,
                content_x_offset,
                content_y_offset,
                205,
                58,
                args.step_timeout_seconds,
            )
            report["rawLogs"].append(f"prep_tab_calibrate: {prep.get('method', 'unknown')}")
            if args.interaction_delay > 0:
                time.sleep(max(0.2, args.interaction_delay))
        except Exception as exc:
            report["rawLogs"].append(f"prep_tab_calibrate failed: {safe_message(exc)}")

        for case in cases:
            ensure_run_budget(start_monotonic, args.max_run_seconds, case.test_id)
            before_path = out_dir / f"{case.test_id}_before.png"
            after_path = out_dir / f"{case.test_id}_after.png"
            before_img = call_with_timeout(
                args.step_timeout_seconds,
                f"{case.test_id} before screenshot",
                screenshot_image,
                driver,
                before_path,
            )
            interaction_method = ""
            interaction_warnings: list[str] = []
            interaction_error = ""

            try:
                click_meta = click_rel(
                    driver,
                    click_origin_x,
                    click_origin_y,
                    content_x_offset,
                    content_y_offset,
                    case.click_x,
                    case.click_y,
                    args.step_timeout_seconds,
                )
                interaction_method = click_meta.get("method", "")
                interaction_warnings.extend(click_meta.get("warnings", []))

                if case.kind == "text":
                    type_meta = type_replace(driver, case.text_value, args.step_timeout_seconds)
                    interaction_method = f"{interaction_method}+{type_meta.get('method', '')}"
                    interaction_warnings.extend(type_meta.get("warnings", []))
            except Exception as exc:
                interaction_error = safe_message(exc)
                report["rawLogs"].append(f"{case.test_id} interaction failed: {interaction_error}")

            if args.interaction_delay > 0:
                time.sleep(args.interaction_delay)

            after_img = call_with_timeout(
                args.step_timeout_seconds,
                f"{case.test_id} after screenshot",
                screenshot_image,
                driver,
                after_path,
            )

            roi_origin_x = click_origin_x if screenshot_scope == "desktop" else 0
            roi_origin_y = click_origin_y if screenshot_scope == "desktop" else 0
            roi_x_points = roi_origin_x + case.roi_x + content_x_offset
            roi_y_points = roi_origin_y + case.roi_y + content_y_offset
            roi_w_points = case.roi_w
            roi_h_points = case.roi_h

            roi_x_px = int(round(roi_x_points * scale_x))
            roi_y_px = int(round(roi_y_points * scale_y))
            roi_w_px = int(round(roi_w_points * scale_x))
            roi_h_px = int(round(roi_h_points * scale_y))

            roi_box = clamp_box(
                (roi_x_px, roi_y_px, roi_x_px + roi_w_px, roi_y_px + roi_h_px),
                before_img.width,
                before_img.height,
            )
            score = diff_score(before_img, after_img, roi_box)
            result = "PASS" if (interaction_error == "" and score > case.threshold) else "FAIL"

            summary_lines.append(
                "\t".join(
                    [
                        case.test_id,
                        result,
                        f"{score:.6f}",
                        f"{case.threshold:.2f}",
                        f"{roi_box[0]},{roi_box[1]},{roi_box[2]},{roi_box[3]}",
                        f"{before_path}|{after_path}",
                        interaction_method or "n/a",
                        ("; ".join(interaction_warnings + ([interaction_error] if interaction_error else [])))
                        or "-",
                    ]
                )
            )

            report["tests"].append(
                {
                    "test_id": case.test_id,
                    "kind": case.kind,
                    "result": result,
                    "threshold": case.threshold,
                    "diff_score": score,
                    "roi_box_px": roi_box,
                    "click_point_rel": [case.click_x, case.click_y],
                    "roi_rel_points": [case.roi_x, case.roi_y, case.roi_w, case.roi_h],
                    "interaction_method": interaction_method or "n/a",
                    "interaction_warnings": interaction_warnings,
                    "interaction_error": interaction_error,
                }
            )

        pass_count = sum(1 for t in report["tests"] if t["result"] == "PASS")
        total_count = len(report["tests"])
        report["result"] = {
            "pass_count": pass_count,
            "total_count": total_count,
            "status": "PASS" if pass_count == total_count else "FAIL",
        }
        exit_code = 0 if pass_count == total_count else 1
    except Exception as exc:
        report["errors"].append(
            {
                "type": exc.__class__.__name__,
                "message": safe_message(exc),
                "traceback": traceback.format_exc(),
            }
        )
        summary_lines.append(f"SESSION\tFAIL\t0.000000\t0.00\t-\t-\t-\t{safe_message(exc)}")
        exit_code = 2
    finally:
        summary_path.write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
        report["finished_at"] = iso_now()
        report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
        if driver is not None:
            try:
                driver.quit()
            except Exception:
                pass

    print(f"Summary: {summary_path}")
    print(f"Report:  {report_path}")
    status = report.get("result", {}).get("status", "FAIL")
    if status == "PASS":
        print("RESULT: PASS")
    else:
        print("RESULT: FAIL")
    return exit_code


if __name__ == "__main__":
    raise SystemExit(run())
