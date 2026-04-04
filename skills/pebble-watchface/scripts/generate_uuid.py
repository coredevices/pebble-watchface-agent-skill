#!/usr/bin/env python3
"""
Generate a UUID for a Pebble watchface

Usage:
    python generate_uuid.py
"""

import uuid

def main():
    new_uuid = str(uuid.uuid4())
    print(f"Generated UUID: {new_uuid}")
    print(f"\nUse in package.json:")
    print(f'  "uuid": "{new_uuid}"')

if __name__ == '__main__':
    main()
