#!/usr/bin/env python3
"""
Pebble Watchface Project Creator

Creates a new Pebble watchface project with the correct structure
and configuration files.

Usage:
    python create_project.py <project_name> [options]

Options:
    --animated    Use animated watchface template
    --static      Use static/analog watchface template
    --weather     Use weather watchface template (includes pkjs)
    --author      Author name
    --display     Display name for the watchface
"""

import os
import sys
import argparse
import uuid
import shutil
from pathlib import Path


def slugify(name):
    """Convert name to a valid slug"""
    return name.lower().replace(' ', '-').replace('_', '-')


def generate_uuid():
    """Generate a random UUID for the watchface"""
    return str(uuid.uuid4())


def create_package_json(project_path, name, display_name, author, template_type='animated'):
    """Create package.json file"""
    content = {
        "name": slugify(name),
        "author": author,
        "version": "1.0.0",
        "keywords": ["pebble-app"],
        "private": True,
        "dependencies": {},
        "pebble": {
            "displayName": display_name,
            "uuid": generate_uuid(),
            "sdkVersion": "3",
            "enableMultiJS": True,
            "targetPlatforms": ["emery"],
            "watchapp": {
                "watchface": True
            },
            "resources": {
                "media": []
            }
        }
    }

    # Weather projects need messageKeys and location capability
    if template_type == 'weather':
        content["pebble"]["capabilities"] = ["location"]
        content["pebble"]["messageKeys"] = ["TEMPERATURE", "CONDITIONS", "REQUEST_WEATHER"]

    import json
    with open(project_path / 'package.json', 'w') as f:
        json.dump(content, f, indent=2)

    print(f"  Created package.json")


def create_wscript(project_path):
    """Create wscript build file"""
    content = """#
# Pebble wscript build configuration
#
import os.path

top = '.'
out = 'build'


def options(ctx):
    ctx.load('pebble_sdk')


def configure(ctx):
    ctx.load('pebble_sdk')


def build(ctx):
    ctx.load('pebble_sdk')

    build_worker = os.path.exists('worker_src')
    binaries = []

    cached_env = ctx.env
    for platform in ctx.env.TARGET_PLATFORMS:
        ctx.env = ctx.all_envs[platform]
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf = '{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_build(source=ctx.path.ant_glob('src/c/**/*.c'),
                      target=app_elf, bin_type='app')

        if build_worker:
            worker_elf = '{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': platform, 'app_elf': app_elf,
                             'worker_elf': worker_elf})
            ctx.pbl_build(source=ctx.path.ant_glob('worker_src/c/**/*.c'),
                          target=worker_elf, bin_type='worker')
        else:
            binaries.append({'platform': platform, 'app_elf': app_elf})
    ctx.env = cached_env

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries,
                   js=ctx.path.ant_glob(['src/pkjs/**/*.js',
                                         'src/pkjs/**/*.json']),
                   js_entry_file='src/pkjs/index.js')
"""
    with open(project_path / 'wscript', 'w') as f:
        f.write(content)

    print(f"  Created wscript")


def create_gitignore(project_path):
    """Create .gitignore file"""
    content = """# Build artifacts
build/

# IDE files
.vscode/
.idea/
*.swp
*.swo
*~

# OS files
.DS_Store
Thumbs.db

# Pebble SDK
.pebble-sdk/
"""
    with open(project_path / '.gitignore', 'w') as f:
        f.write(content)

    print(f"  Created .gitignore")


def copy_template(project_path, template_type, skill_path):
    """Copy the appropriate template file"""
    c_templates = {
        'animated': 'animated-watchface.c',
        'static': 'static-watchface.c',
        'weather': 'weather-watchface.c'
    }

    template_file = c_templates.get(template_type, 'animated-watchface.c')
    template_path = skill_path / 'templates' / template_file

    # C project structure
    c_dir = project_path / 'src' / 'c'
    c_dir.mkdir(parents=True, exist_ok=True)

    if template_path.exists():
        shutil.copy(template_path, c_dir / 'main.c')
        print(f"  Created src/c/main.c from {template_file}")
    else:
        with open(c_dir / 'main.c', 'w') as f:
            f.write('#include <pebble.h>\n\nint main(void) {\n    app_event_loop();\n    return 0;\n}\n')
        print(f"  Created src/c/main.c (minimal)")

    # Weather template also needs pkjs
    if template_type == 'weather':
        pkjs_dir = project_path / 'src' / 'pkjs'
        pkjs_dir.mkdir(parents=True, exist_ok=True)
        pkjs_template = skill_path / 'templates' / 'pkjs-weather.js'
        if pkjs_template.exists():
            shutil.copy(pkjs_template, pkjs_dir / 'index.js')
            print(f"  Created src/pkjs/index.js from pkjs-weather.js")
        else:
            with open(pkjs_dir / 'index.js', 'w') as f:
                f.write("// PebbleKit JS\nPebble.addEventListener('ready', function() {});\n")
            print(f"  Created src/pkjs/index.js (minimal)")


def main():
    parser = argparse.ArgumentParser(description='Create a new Pebble watchface project')
    parser.add_argument('name', help='Project name')
    parser.add_argument('--animated', action='store_true', help='Use animated template')
    parser.add_argument('--static', action='store_true', help='Use static/analog template')
    parser.add_argument('--weather', action='store_true', help='Use weather template (includes pkjs)')
    parser.add_argument('--author', default='Your Name', help='Author name')
    parser.add_argument('--display', default=None, help='Display name')
    parser.add_argument('--output', '-o', default='.', help='Output directory')

    args = parser.parse_args()

    # Determine template type
    if args.static:
        template_type = 'static'
    elif args.weather:
        template_type = 'weather'
    else:
        template_type = 'animated'

    display_name = args.display or args.name

    # Create project directory
    output_path = Path(args.output).resolve()
    project_path = output_path / slugify(args.name)

    if project_path.exists():
        print(f"Error: Directory already exists: {project_path}")
        sys.exit(1)

    print(f"\nCreating Pebble watchface project: {args.name}")
    print(f"Template: {template_type}")
    print(f"Location: {project_path}\n")

    # Create directories
    project_path.mkdir(parents=True)
    (project_path / 'resources' / 'fonts').mkdir(parents=True)
    (project_path / 'resources' / 'images').mkdir(parents=True)

    # Find skill path (for templates)
    script_path = Path(__file__).resolve().parent
    skill_path = script_path.parent

    # Create files
    create_package_json(project_path, args.name, display_name, args.author, template_type)
    create_wscript(project_path)
    create_gitignore(project_path)
    copy_template(project_path, template_type, skill_path)

    print(f"\n✓ Project created successfully!")
    print(f"\nNext steps:")
    print(f"  1. cd {project_path}")
    print(f"  2. Edit src/c/main.c (or src/pkjs/index.js)")
    print(f"  3. pebble build")
    print(f"  4. pebble install --emulator emery")


if __name__ == '__main__':
    main()
