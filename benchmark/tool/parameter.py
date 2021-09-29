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

def mk_take(e1, e2):
    return {'take': {'e1': e1, 'e2': e2}}

def mk_drop(e1, e2):
    return {'drop': {'e1': e1, 'e2': e2}}

with open('parameter_schema.json', 'r') as f:
    parameter_schema = json.loads(f.read())

#p = json.loads('{"take": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": 3}]] }, "e2": {"value": [] } }}')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ]] }')
#v1 = '{"value": [[ {"key": "x", "val": "3"} ]] }'
#p = json.loads('{"append": { "e1": ' +  v1 + ', "e2": ' + v1 + ' } }')
p1 = mk_parameter('foo', 'bar')
p2 = mk_parameter('foo', 321.0)
p3 = mk_append(p1, p2)
p4 = mk_parameters('baz', [1,2,3])
p = mk_take(p3, p4)
print(p)
jsonschema.validate(p, parameter_schema)
pretty_print_json(p)

   
#{"append": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }, "e2": {"value": [] } }}
