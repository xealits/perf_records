'''
dump CSV with the long format of all the data for each analysis records
'''

from bs4 import BeautifulSoup
import csv
from collections import defaultdict
from pathlib import Path
from sys import argv

fname = Path(argv[1])

with open(fname, 'r') as f:
    example_s = f.read()
    
soup = BeautifulSoup(example_s, 'html.parser')
soup.select(".perf_records")[0].select("summary")[0]
soup.select(".perf_records")[0].find("summary").find("var").get_text(strip=True)
'testing X'

def direct_text(soup_elem):
    return ''.join([x for x in soup_elem.contents if isinstance(x, str)])

def direct_text_default(soup_elem, default = "NA"):
    if soup_elem is None:
        return default
    else:
        return direct_text(soup_elem).strip()

def parse_param(param):
    # var_name = direct_text(param.find("dfn"))
    var_name = param.get("data-name")
    assert var_name is not None, f"no data-name in record var param {param}"
    var_data = direct_text_default(param.find("data"))
    return var_name, {"value": var_data}

def parse_record(record_soup):
    # a record can be just a var or details-summary
    assert record_soup.name in ("var", "details"), f"input mismatch: {record_soup}"

    if record_soup.name == "var":
        return dict([parse_param(record_soup)])

    # else it is a details-record with subrecords under perf_records_nest
    name, params = parse_param(record_soup.find("summary").find("var"))
    rec_dict = {name: params}

    condition = record_soup.find("summary").find("div")
    if condition:
        conds = condition.find_all(recursive=False)
    else:
        conds = []

    for cond in conds:
        params.update(parse_record(cond))

    subrecs = record_soup.find("div", recursive=False).find_all(recursive=False)
    for rec in subrecs:
        params.update(parse_record(rec))

    return rec_dict

# at the top level all .perf_records have the analysis ID
analysis_records = defaultdict(list)
for rec in soup.select(".perf_records"):
    children = rec.find_all(recursive=False)
    #print(list(parse_record(soup) for soup in children))
    assert len(children) == 1
    anarec = parse_record(children[0])["analysisID"]
    analysis_id = anarec["value"]
    anarec.pop("value")
    analysis_records[analysis_id].append(anarec)

# let's just handle the first one now:
records = list(analysis_records.values())[0]

# find all keys
def flat_rec(rec_dict, topname = None):
    flat_dict = {}
    for key, v_dict in rec_dict.items():
        assert isinstance(v_dict, dict)
        flat_name = key if topname is None else f"{topname}.{key}"
        value = v_dict["value"]
        flat_dict[flat_name] = value

        v_dict.pop("value")
        if len(v_dict) > 0:
            flat_dict.update(flat_rec(v_dict, flat_name))

    return flat_dict

flat_records = list(flat_rec(rec) for rec in records)
all_keys = set.union(*[set(d.keys()) for d in flat_records])

with open(f'{fname.stem}.csv', 'w', newline='') as csvfile:
    fieldnames = all_keys
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for rec in flat_records:
        writer.writerow(rec)

