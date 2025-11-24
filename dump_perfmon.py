#!/usr/bin/env python
# sorry, no proper Python project yet

import argparse
import json, csv
from pathlib import Path
import logging
import sys

logger = logging.getLogger(__name__)
#logging.basicConfig(level=logging.INFO)
logger.setLevel(logging.INFO)
stderr_handler = logging.StreamHandler(sys.stderr)
stderr_handler.setLevel(logging.INFO)
logger.addHandler(stderr_handler)

def parse_args():
    parser = argparse.ArgumentParser(description='Dump perfmon records')
    parser.add_argument('model', help='Intel CPU model')
    parser.add_argument('map_symbol', help='Event map symbol name in the generated header')
    return parser.parse_args()

def update_events_dict(dest, events, event_type):
    for event in events:
        event_name = event['EventName']
        if event_name in dest:
            logger.warning(f"duplicate event {event_name} under {event_type}")
            event_name = f"{event_type}.{event_name}"
        dest[event_name] = event

def get_cpu_events(model, perfmon_dir=Path('./intel_perfmon')):
    cpu_events = {}

    with open('intel_perfmon/mapfile.csv') as f:
        mapreader = csv.DictReader(f)

        for row in mapreader:
            if row['Family-model'] != model:
                continue

            perfmon_path = Path(row['Filename'].lstrip('/'))
            event_type = row['EventType']
            json_path = perfmon_dir / perfmon_path
            #cpu_events[event_type] = json.load(open(json_path))
            events_json = json.load(open(json_path))
            update_events_dict(cpu_events, events_json["Events"], event_type)

    return cpu_events

header_template = """
#pragma once
#include "events_map.hpp"

PerfEventMap_t {map_symbol} = {{
  {event_data}
}};
"""

map_data_template = """\
{{ "{EventName}", \
{{.perf_config = RawEventConfig({EventCode}, {UMask}), \
.perf_type = PERF_TYPE_RAW}} }}"""

def main():
    args = parse_args()

    cpu_events = get_cpu_events(args.model)
    if len(cpu_events) == 0:
        print(f"No perfmon data found for model {args.model}")
        return 1

    #print(json.dumps(cpu_events, indent=4))
    #print("done")
    header_events_strings = []
    for final_event_name, event in cpu_events.items():
        if ',' in event['UMask']:
            logger.warning(f"skipping event {final_event_name} with multiple UMask values {event['UMask']}")
            continue
        event_data = {}
        event_data.update(event)
        event_data['EventName'] = final_event_name
        event_string = map_data_template.format(**event_data)
        header_events_strings.append(event_string)

    event_data_string = ',\n  '.join(header_events_strings)
    #print(escape_braces(event_data_string))
    print(header_template.format(map_symbol=args.map_symbol,
                                 event_data=event_data_string))

    logger.info(f"Dumped {len(cpu_events)} events for model {args.model}")

if __name__ == '__main__':
    main()
