import jsonschema
import simplejson as json

def pretty_print_json(j):
    print(json.dumps(j, indent=4, sort_keys=True))

def mk_unit():
    return { 'value': [ [ ] ] }

def mk_parameter(key, val):
    return { 'value': [ [ {'key': key, 'val': val } ] ] }

def mk_parameters(key, vals):
    return { 'value': [ [ {'key': key, 'val': val } ] for val in vals ] }

def mk_append(e1, e2):
    return {'append': {'e1': e1, 'e2': e2}}

def mk_cross(e1, e2):
    return {'cross': {'e1': e1, 'e2': e2}}

def mk_take_kvp(e1, e2):
    return {'take_kvp': {'e1': e1, 'e2': e2}}

def mk_drop_kvp(e1, e2):
    return {'drop_kvp': {'e1': e1, 'e2': e2}}

def does_row_contain(r, kvp):
    res = False
    for kvpr in r:
        if kvp == kvpr:
            return True
    return res

def do_rows_match(r, rm):
    for kvp in rm:
        if not(does_row_contain(r, kvp)):
            return False
    return True

def does_row_match_any(r, rms):
    for rm in rms:
        if do_rows_match(r, rm):
            return True
    return False

def eval(e):
    k = list(e.keys())[0]
    if k == 'value':
        return e
    v1 = eval(e[k]['e1'])
    v2 = eval(e[k]['e2'])
    if k == 'append':
        return { 'value': v1['value'] + v2['value'] }
    if k == 'cross':
        # todo: check and report any rows that contain duplicate keys
        return { 'value': [ r1 + r2 for r2 in v2['value'] for r1 in v1['value'] ] }
    if k == 'take_kvp':
        return { 'value': [ r for r in v1['value'] if does_row_match_any(r, v2['value']) ] }
    if k == 'drop_kvp':
        return { 'value': [ r for r in v1['value'] if not(does_row_match_any(r, v2['value'])) ] }
    assert(False)

with open('parameter_schema.json', 'r') as f:
    parameter_schema = json.loads(f.read())

#p = json.loads('{"take_kvp": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": 3}]] }, "e2": {"value": [] } }}')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ]] }')
#v1 = '{"value": [[ {"key": "x", "val": "3"} ]] }'
#p = json.loads('{"append": { "e1": ' +  v1 + ', "e2": ' + v1 + ' } }')
p1 = mk_parameter('foo1', 'bar')
p2 = mk_parameter('foo2', 321.0)
p3 = mk_parameter('baz', 'xx')
p4 = mk_parameters('baz', [1,2,3])
p = mk_cross(p3, p3)
q = mk_cross(p1, mk_append(p2, p3))
pretty_print_json(eval(q))
q = mk_take_kvp(q, p3)
pretty_print_json(q)
p = eval(q)
jsonschema.validate(p, parameter_schema)
pretty_print_json(p)

   
#{"append": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }, "e2": {"value": [] } }}
