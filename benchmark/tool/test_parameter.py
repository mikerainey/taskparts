from parameter import *

#p = json.loads('{"take_kvp": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": 3}]] }, "e2": {"value": [] } }}')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ]] }')
#v1 = '{"value": [[ {"key": "x", "val": "3"} ]] }'
#p = json.loads('{"append": { "e1": ' +  v1 + ', "e2": ' + v1 + ' } }')
p1 = mk_parameter('foo1', 'bar')
p2 = mk_parameter('foo2', 321.0)
p3 = mk_parameter('baz', 555)
p4 = mk_parameters('baz', [1,2,3])
p = mk_cross(p3, p3)
q = mk_cross(p1, mk_append(p2, p3))
pretty_print_json(eval(q))
q = mk_take_ktp(q, mk_parameter('baz', 'number'))
pretty_print_json(q)
p = eval(q)
jsonschema.validate(p, parameter_schema)
pretty_print_json(p)

   
#{"append": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }, "e2": {"value": [] } }}
