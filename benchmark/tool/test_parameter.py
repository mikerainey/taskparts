from parameter import *

def pretty_print_json(j):
    print(json.dumps(j, indent=2))

#p = json.loads('{"take_kvp": {"e1": {"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": 3}]] }, "e2": {"value": [] } }}')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ], [{"key": "x", "val": "3"},{"key": "x", "val": "3"}]] }')
#p = json.loads('{"value": [[ {"key": "x", "val": "3"} ]] }')
#v1 = '{"value": [[ {"key": "x", "val": "3"} ]] }'
#p = json.loads('{"append": { "e1": ' +  v1 + ', "e2": ' + v1 + ' } }')
p1 = mk_parameter('foo1', 'bar')
p2 = mk_parameter('foo2', 321.0)
p3 = mk_append(p1,p2)
p0 = mk_cross(p3, mk_append(mk_parameter('yyy', 222), mk_parameter('xxx', 333)))
pretty_print_json(eval(p0))
print('')

p3 = mk_parameter('baz', 555)
p4 = mk_parameters('baz', [1,2,3])
p5 = mk_parameter('bbz', 1555)
p = mk_cross(p3, p3)
q = mk_cross(p1, mk_cross(p5, mk_append(p2, p3)))
q = eval(q)
#pretty_print_json(eval(q))
#q = mk_take_ktp(q, mk_parameter('baz', 'number'))
#pretty_print_json(q)
#p = eval(q)
#jsonschema.validate(p, parameter_schema)
#pretty_print_json(p)

def does_row_contain_key_slow(r, key):
    for kvpr in r:
        if kvpr['key'] == key:
            return True
    return False

def does_row_contain_kvp_slow(r, kvp):
    for kvpr in r:
        if kvp == kvpr:
            return True
    return False

def does_row_contain_ktp_slow(r, ktp):
    for kvpr in r:
        if kvpr['key'] == ktp['key']:
            if type(kvpr['val']) is str and ktp['val'] == 'string':
                return True
            if is_number(kvpr['val']) and ktp['val'] == 'number':
                return True
    return False

def check_does_row_contain_kvp(r, kvp):
    ans_ref = does_row_contain_kvp_slow(r, kvp)
    ans_chk = does_row_contain_kvp(r, kvp)
    return ans_ref == ans_chk

pretty_print_json(eval(q))
assert(check_does_row_contain_kvp([ {"key": "x", "val": "3"} ],  {"key": "x", "val": "3"}))
assert(check_does_row_contain_kvp([ {"key": "x2", "val": "3"} ],  {"key": "x", "val": "3"}))
assert(check_does_row_contain_kvp([ {"key": "x2", "val": "33"} ],  {"key": "x", "val": "3"}))
assert(check_does_row_contain_kvp([ {"key": "x", "val": "3"} ],  {"key": "y", "val": "3"}))
assert(check_does_row_contain_kvp([ {"key": "x", "val": "3"} ],  {"key": "y", "val": 3}))
assert(check_does_row_contain_kvp([ {"key": "x", "val": "3"} ],  {"key": "x", "val": 3}))
assert(check_does_row_contain_kvp([ {"key": "d", "val": 3}, {"key": "x", "val": "3"} ],  {"key": "x", "val": 3}))
assert(check_does_row_contain_kvp([ {"key": "d", "val": 3}, {"key": "x", "val": "3"} ],  {"key": "d", "val": 3}))
assert(check_does_row_contain_kvp([ {"key": "d", "val": 3}, {"key": "x", "val": "3"} ],  {"key": "e", "val": 3}))
assert(check_does_row_contain_kvp([ {"key": "d", "val": 3}, {"key": "e", "val": "3"}, {"key": "x", "val": "3"} ],  {"key": "e", "val": 3}))
