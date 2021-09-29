import jsonschema
import simplejson as json

with open('parameter_schema.json', 'r') as f:
    parameter_schema = json.loads(f.read())

p = json.loads('{"take": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }, "e2": {"value": [] } }}')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }')
jsonschema.validate(p, parameter_schema)
print(p)

   
#{"append": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }, "e2": {"value": [] } }}
