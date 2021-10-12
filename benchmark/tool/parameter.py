import jsonschema
import simplejson as json

# Smart constructors
# ==================

# shouldn't unit be [[]]?
def mk_unit():
    return { 'value': [ ] }

def mk_parameter(key, val):
    return { 'value': [ [ {'key': key, 'val': val } ] ] }

def mk_parameters(key, vals):
    return { 'value': [ [ {'key': key, 'val': val } ] for val in vals ] }

def mk_append(e1, e2):
    return {'append': {'e1': e1, 'e2': e2}}

def mk_append_sequence(es):
    r = mk_unit()
    for e in es:
        r = mk_append(r, e)
    return r

def mk_cross(e1, e2):
    return {'cross': {'e1': e1, 'e2': e2}}

def mk_cross_sequence(es):
    r = mk_unit()
    for e in es:
        r = mk_cross(r, e)
    return r

def mk_take_kvp(e1, e2):
    return {'take_kvp': {'e1': e1, 'e2': e2}}

def mk_drop_kvp(e1, e2):
    return {'drop_kvp': {'e1': e1, 'e2': e2}}

def mk_take_ktp(e1, e2):
    return {'take_ktp': {'e1': e1, 'e2': e2}}

def mk_drop_ktp(e1, e2):
    return {'drop_ktp': {'e1': e1, 'e2': e2}}

# Queries
# =======

def do_rows_match(r, rm, does_row_contain):
    for kvp in rm:
        if not(does_row_contain(r, kvp)):
            return False
    return True

def does_row_match_any(r, rms, does_row_contain):
    for rm in rms:
        if do_rows_match(r, rm, does_row_contain):
            return True
    return False

def does_row_contain_kvp(r, kvp):
    for kvpr in r:
        if kvp == kvpr:
            return True
    return False

def is_number(v):
    return (type(v) is int) or (type(v) is float)

def does_row_contain_ktp(r, ktp):
    for kvpr in r:
        if kvpr['key'] == ktp['key']:
            if type(kvpr['val']) is str and ktp['val'] == 'string':
                return True
            if is_number(kvpr['val']) and ktp['val'] == 'number':
                return True
    return False

def does_row_contain_key(r, key):
    for kvpr in r:
        if kvpr['key'] == key:
            return True
    return False

def select_from_expr_by_key(expr, k):
    vals = []
    for r in eval(expr)['value']:
        for kvp in r:
            if kvp['key'] == k:
                vals += [kvp['val']]
    return vals

# Evaluation
# ==========

with open('parameter_schema.json', 'r') as f:
    parameter_schema = json.loads(f.read())

def validate_parameter(p):
    jsonschema.validate(p, parameter_schema)

def eval(e):
    validate_parameter(e)
    r = eval_rec(e)
    validate_parameter(r)
    return r

def check_for_duplicate_keys(v1, v2):
    for r2 in v2['value']:
        for kvp in r2:
            for r1 in v1['value']:
                assert(not(does_row_contain_key(r1, kvp['key'])))

def get_first_key_in_dictionary(d):
    return list(d.keys())[0]
                
def eval_rec(e):
    k = get_first_key_in_dictionary(e)
    if k == 'value':
        return e
    v1 = eval(e[k]['e1'])
    v2 = eval(e[k]['e2'])
    v1rs = v1['value']
    v2rs = v2['value']
    if k == 'append':
        return { 'value': v1rs + v2rs }
    if k == 'cross':
        check_for_duplicate_keys(v1, v2)
        return { 'value': [ r1 + r2 for r2 in v2rs for r1 in v1rs ] }
    if k == 'take_kvp':
        return { 'value': [ r for r in v1rs
                            if does_row_match_any(r, v2rs,
                                                  does_row_contain_kvp) ] }
    if k == 'drop_kvp':
        return { 'value': [ r for r in v1rs
                            if not(does_row_match_any(r, v2rs,
                                                      does_row_contain_kvp)) ] }
    if k == 'take_ktp':
        return { 'value': [ r for r in v1rs
                            if does_row_match_any(r, v2rs,
                                                  does_row_contain_ktp) ] }
    if k == 'drop_ktp':
        return { 'value': [ r for r in v1rs
                            if not(does_row_match_any(r, v2rs,
                                                      does_row_contain_ktp)) ] }
    assert(False)

# Misc

def row_to_dictionary(row):
    return dict(zip([ kvp['key'] for kvp in row ],
                    [ kvp['val'] for kvp in row ]))

def dictionary_to_row(dct):
    return [ {'key': k, 'val': v} for k, v in dct.items() ]

