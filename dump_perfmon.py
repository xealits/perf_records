#!/usr/bin/env python
# sorry, no proper Python project yet

import argparse
import json, csv
from pathlib import Path

def parse_args():
    parser = argparse.ArgumentParser(description='Dump perfmon records')
    parser.add_argument('model', help='Intel CPU model')
    return parser.parse_args()

def get_cpu_events(model, perfmon_dir=Path('./intel_perfmon')):
    cpu_events = {}

    with open('intel_perfmon/mapfile.csv') as f:
        mapreader = csv.DictReader(f)

        for row in mapreader:
            if row['Family-model'] == model:
                perfmon_path = Path(row['Filename'].lstrip('/'))
                event_type = row['EventType']
                json_path = perfmon_dir / perfmon_path
                print(perfmon_dir)
                print(perfmon_dir / 'foo')
                print(perfmon_path)
                print(perfmon_dir / perfmon_path)
                print(json_path)
                cpu_events[event_type] = json.load(open(json_path))

    return cpu_events

def main():
    args = parse_args()

    cpu_events = get_cpu_events(args.model)
    if len(cpu_events) == 0:
        print(f"No perfmon data found for model {args.model}")
        return 1

    print(json.dumps(cpu_events, indent=4))

if __name__ == '__main__':
    main()
