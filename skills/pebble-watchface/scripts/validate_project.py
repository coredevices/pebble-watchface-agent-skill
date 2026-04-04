#!/usr/bin/env python3
"""
Pebble Watchface Project Validator

Validates the structure and configuration of a Pebble watchface project
before building. Checks for common issues and provides helpful feedback.

Usage:
    python validate_project.py /path/to/watchface

Exit codes:
    0 - All validations passed
    1 - Validation errors found
"""

import os
import sys
import json
import re
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output"""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def print_status(status, message):
    """Print a status message with color"""
    if status == 'ok':
        print(f"  {Colors.GREEN}✓{Colors.RESET} {message}")
    elif status == 'error':
        print(f"  {Colors.RED}✗{Colors.RESET} {message}")
    elif status == 'warning':
        print(f"  {Colors.YELLOW}!{Colors.RESET} {message}")
    elif status == 'info':
        print(f"  {Colors.BLUE}ℹ{Colors.RESET} {message}")


def validate_file_exists(project_path, filename, required=True):
    """Check if a required file exists"""
    file_path = project_path / filename
    if file_path.exists():
        print_status('ok', f"{filename} exists")
        return True
    else:
        status = 'error' if required else 'warning'
        print_status(status, f"{filename} {'not found' if required else 'not found (optional)'}")
        return False


def validate_package_json(project_path):
    """Validate package.json structure and contents"""
    package_path = project_path / 'package.json'
    errors = []

    if not package_path.exists():
        return ['package.json not found']

    try:
        with open(package_path, 'r') as f:
            pkg = json.load(f)
    except json.JSONDecodeError as e:
        return [f'package.json has invalid JSON: {e}']

    # Check required fields
    required_fields = ['name', 'pebble']
    for field in required_fields:
        if field not in pkg:
            errors.append(f'Missing required field: {field}')

    # Check pebble configuration
    if 'pebble' in pkg:
        pebble = pkg['pebble']

        if 'uuid' not in pebble:
            errors.append('Missing pebble.uuid')
        elif not re.match(r'^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$',
                         pebble.get('uuid', ''), re.IGNORECASE):
            errors.append('Invalid UUID format')

        if 'displayName' not in pebble:
            errors.append('Missing pebble.displayName')

        if 'sdkVersion' not in pebble:
            print_status('warning', 'pebble.sdkVersion not specified (will use default)')

        if 'watchapp' not in pebble:
            errors.append('Missing pebble.watchapp configuration')
        elif pebble.get('watchapp', {}).get('watchface') is not True:
            print_status('warning', 'pebble.watchapp.watchface is not true (not marked as watchface)')

        # Check target platforms
        platforms = pebble.get('targetPlatforms', [])
        valid_platforms = ['aplite', 'basalt', 'chalk', 'diorite', 'emery', 'flint', 'gabbro']
        for p in platforms:
            if p not in valid_platforms:
                errors.append(f'Invalid target platform: {p}')

        if not platforms:
            print_status('warning', 'No target platforms specified (will build for all)')

    if errors:
        for error in errors:
            print_status('error', f'package.json: {error}')
    else:
        print_status('ok', 'package.json is valid')

    return errors


def validate_source_structure(project_path):
    """Validate source file structure"""
    errors = []

    src_path = project_path / 'src'
    if not src_path.exists():
        errors.append('src/ directory not found')
        return errors

    # Check for C source files
    c_sources = list((project_path / 'src').rglob('*.c'))
    if c_sources:
        print_status('ok', f'Found {len(c_sources)} C source file(s)')

        # Check for main.c specifically
        main_files = [f for f in c_sources if f.name == 'main.c']
        if main_files:
            print_status('ok', 'main.c found')
        else:
            print_status('warning', 'No main.c found (uncommon)')
    else:
        # Check for JavaScript sources
        js_sources = list((project_path / 'src').rglob('*.js'))
        if js_sources:
            print_status('ok', f'Found {len(js_sources)} JavaScript source file(s)')
        else:
            errors.append('No source files found (no .c or .js files)')

    return errors


def validate_c_source(project_path):
    """Validate C source code for common issues"""
    warnings = []

    c_sources = list((project_path / 'src').rglob('*.c'))
    for source_file in c_sources:
        try:
            with open(source_file, 'r') as f:
                content = f.read()

            # Check for pebble.h include
            if '#include <pebble.h>' not in content:
                warnings.append(f'{source_file.name}: Missing #include <pebble.h>')

            # Check for main function
            if 'int main' in content:
                print_status('ok', f'{source_file.name}: Has main() function')
            elif source_file.name == 'main.c':
                warnings.append(f'{source_file.name}: No main() function found')

            # Check for common issues
            if 'float ' in content or 'double ' in content:
                print_status('warning', f'{source_file.name}: Uses floating point (not recommended)')

            if 'malloc(' in content or 'calloc(' in content:
                print_status('info', f'{source_file.name}: Uses dynamic memory allocation')

            # Check for proper cleanup patterns
            if 'window_create' in content and 'window_destroy' not in content:
                warnings.append(f'{source_file.name}: window_create without window_destroy')

            if 'gpath_create' in content and 'gpath_destroy' not in content:
                warnings.append(f'{source_file.name}: gpath_create without gpath_destroy')

        except Exception as e:
            warnings.append(f'Could not analyze {source_file.name}: {e}')

    for warning in warnings:
        print_status('warning', warning)

    return warnings


def validate_resources(project_path):
    """Check resources directory"""
    resources_path = project_path / 'resources'

    if resources_path.exists():
        print_status('ok', 'resources/ directory exists')

        # Check for fonts
        font_files = list(resources_path.rglob('*.ttf')) + list(resources_path.rglob('*.otf'))
        if font_files:
            print_status('info', f'Found {len(font_files)} custom font(s)')

        # Check for images
        image_files = list(resources_path.rglob('*.png')) + list(resources_path.rglob('*.pbi'))
        if image_files:
            print_status('info', f'Found {len(image_files)} image resource(s)')
    else:
        print_status('info', 'No resources/ directory (using system fonts only)')


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} /path/to/watchface/project")
        sys.exit(1)

    project_path = Path(sys.argv[1]).resolve()

    print(f"\n{Colors.BOLD}Validating Pebble Watchface Project{Colors.RESET}")
    print(f"Project: {project_path}\n")

    if not project_path.exists():
        print_status('error', f'Project path does not exist: {project_path}')
        sys.exit(1)

    if not project_path.is_dir():
        print_status('error', 'Path is not a directory')
        sys.exit(1)

    all_errors = []

    # File structure checks
    print(f"{Colors.BOLD}Checking file structure...{Colors.RESET}")
    validate_file_exists(project_path, 'package.json', required=True)
    validate_file_exists(project_path, 'wscript', required=True)
    validate_file_exists(project_path, 'appinfo.json', required=False)

    # package.json validation
    print(f"\n{Colors.BOLD}Validating package.json...{Colors.RESET}")
    errors = validate_package_json(project_path)
    all_errors.extend(errors)

    # Source structure validation
    print(f"\n{Colors.BOLD}Checking source structure...{Colors.RESET}")
    errors = validate_source_structure(project_path)
    all_errors.extend(errors)

    # C source validation
    print(f"\n{Colors.BOLD}Analyzing C source code...{Colors.RESET}")
    validate_c_source(project_path)

    # Resources check
    print(f"\n{Colors.BOLD}Checking resources...{Colors.RESET}")
    validate_resources(project_path)

    # Summary
    print(f"\n{Colors.BOLD}Summary{Colors.RESET}")
    if all_errors:
        print(f"{Colors.RED}Found {len(all_errors)} error(s){Colors.RESET}")
        for error in all_errors:
            print(f"  • {error}")
        sys.exit(1)
    else:
        print(f"{Colors.GREEN}All validations passed!{Colors.RESET}")
        print("\nNext steps:")
        print(f"  1. cd {project_path}")
        print("  2. pebble build")
        print("  3. pebble install --emulator basalt")
        sys.exit(0)


if __name__ == '__main__':
    main()
