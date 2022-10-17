import jsonschema
import simplejson as json
from heapq import merge
from operator import itemgetter, attrgetter
from bisect import bisect_left

# For debugging purposes:
def pretty_print_json(j):
    print(json.dumps(j, indent=2))
def ppj(j):
    pretty_print_json(j)

# Smart constructors
# ==================

def mk_nil():
    return { 'value': [ ] }

def mk_unit():
    return { 'value': [ [ ] ] }

def mk_parameter(key, val):
    return { 'value': [ [ {'key': key, 'val': val } ] ] }

def mk_parameters(key, vals):
    return { 'value': [ [ {'key': key, 'val': val } ] for val in vals ] }

def mk_append(e1, e2):
    return {'append': {'e1': e1, 'e2': e2}}

def mk_append_sequence(es):
    r = mk_nil()
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

with open('parameter_schema.json', 'r') as f:
    parameter_schema = json.loads(f.read())

def validate_parameter(p):
    jsonschema.validate(p, parameter_schema)

def check_if_row_is_well_formed(r):
    n = len(r)
    if (n < 2):
        return True
    prev = r[0]
    for i in range(1, n):
        nxt = r[i]
        if not(prev['key'] < nxt['key']):
            return False
        prev = nxt
    return True

def check_if_value_is_well_formed(v):
    if 'value' not in v:
        return False
    validate_parameter(v)
    rows = v['value']
    for r in rows:
        check_if_row_is_well_formed(r)
    return True

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

def index(a, x):
    'Locates the leftmost value in array a exactly equal to x'
    i = bisect_left(a, x)
    if i != len(a) and a[i] == x:
        return i
    return -1

def index_of_key_in_row(r, k):
    return index([kvp['key'] for kvp in r], k)

def does_row_contain_key(r, k):
    return index_of_key_in_row(r, k) != -1

def does_row_contain_kvp(r, kvp):
    i = index_of_key_in_row(r, kvp['key'])
    if i == -1:
        return False
    return kvp['val'] == r[i]['val']
    
def is_number(v):
    return (type(v) is int) or (type(v) is float)

def does_row_contain_ktp(r, ktp):
    i = index_of_key_in_row(r, ktp['key'])
    if i == -1:
        return False
    kvpr = r[i]
    if type(kvpr['val']) is str and ktp['val'] == 'string':
        return True
    if is_number(kvpr['val']) and ktp['val'] == 'number':
        return True
    return False

def select_from_expr_by_key(expr, k):
    vals = []
    for r in eval(expr)['value']:
        i = index_of_key_in_row(r, k)
        if i != -1:
            vals += [r[i]['val']]
    return vals

def get_first_key_in_dictionary(d):
    return list(d.keys())[0]

def val_of_key_in_row(r, k):
    i = index_of_key_in_row(r, k)
    if i == -1:
        return []
    return [r[i]['val']]

# Conversions
# ===========

def mk_tuples_of_row(r):
    return zip([ kvp['key'] for kvp in r ],
               [ kvp['val'] for kvp in r ])

def row_to_dictionary(r):
    return dict(mk_tuples_of_row(r))

def dictionary_to_row(d):
    return [ {'key': k, 'val': v}
             for k, v in (dict(sorted(d.items(), key = itemgetter(0)))).items() ]

def mk_dictionary_of_tuples(ts):
    d = {}
    for x, y in ts:
        d.setdefault(x, []).append(y)
    return d

def merge_list_of_tuples(ts1, ts2):
    return merge(ts1, ts2, key = itemgetter(0))

def cross_rows(r1, r2,
               value_list_combiner =
               lambda vs: vs[0]):
    ts = merge_list_of_tuples(mk_tuples_of_row(r1), mk_tuples_of_row(r2))
    return [ {'key': k, 'val': value_list_combiner(v)}
             for k, v in (mk_dictionary_of_tuples(ts)).items() ]

def mk_expr_of_row(r):
    return { 'value': [ dictionary_to_row(row_to_dictionary(r)) ] }

def collapse_rows(rs):
    ts = []
    for r in rs:
        ts = merge_list_of_tuples(ts, mk_tuples_of_row(r))
    return dictionary_to_row(mk_dictionary_of_tuples(ts))

def genfunc_expr_by_row(e):
    for r in rows_of(e):
        yield mk_expr_of_row(r)

def human_readable_string_of_row(row,
                                 col_separator = ',',
                                 silent_keys = [],
                                 show_keys = True):
    row = [ kvp for kvp in row if kvp['key'] not in silent_keys ]
    s = col_separator.join([ ((str(kvp['key']) + '=') if show_keys else '')
                             +
                             str(kvp['val']) for kvp in row ])
    return s
                                  
def human_readable_string_of_expr(e,
                                  col_separator = ',',
                                  row_separator = ';',
                                  silent_keys = [],
                                  show_keys = True):
    rows = rows_of(e)
    n = len(rows)
    if n == 0:
        s = '<>'
    else:
        s = human_readable_string_of_row(rows[0],
                                         col_separator = col_separator,
                                         show_keys = show_keys,
                                         silent_keys = silent_keys)
    if n == 1:
        return s
    for i in range(1, n):
        s += row_separator + human_readable_string_of_row(rows[i],
                                                          col_separator = col_separator,
                                                          show_keys = show_keys,
                                                          silent_keys = silent_keys)
    return s

# Evaluation
# ==========

# later: write proper error message
def value_list_combiner_1(vs):
    assert(len(vs) == 1)
    return vs[0]

def eval(e):
    validate_parameter(e)
    v = eval_rec(e)
    assert(check_if_value_is_well_formed(v))
    return v

# later: enforce a normal form for values whereby empty rows can occur
# at most once
def eval_rec(e):
    k = get_first_key_in_dictionary(e)
    if k == 'value':
        return e
    v1 = eval(e[k]['e1'])
    v2 = eval(e[k]['e2'])
    v1rs = v1['value']
    v2rs = v2['value']
    if k == 'append':
        vrs = []
        if v1rs == [[]]:
            vrs = v2rs
        elif v2rs == [[]]:
            vrs = v1rs
        else:
            vrs = v1rs + v2rs
        return { 'value': vrs }
    if k == 'cross':
        vr = { 'value': [ cross_rows(r1, r2,
                                     value_list_combiner = value_list_combiner_1)
                          for r2 in v2rs for r1 in v1rs ] }
        return vr
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

def rows_of(e):
    return eval(e)['value']
