import sqlalchemy 
import model
import json
import datetime

def setup_test() :
    engine = sqlalchemy.create_engine("sqlite+pysqlite:///:memory:", echo=True, future=True)
    return engine

class Ingester:
    def __init__(self, engine) -> None:
        self.engine = engine
        pass

    def ingest(self, filepath, machine : model.Machine, exclude=None):
        with open(filepath, 'r') as fp: 
            source = json.load(fp)
            with sqlalchemy.orm.Session(self.engine) as session:
                for run in source["value"]:
                    # Constructing a dictionary 
                    kv = dict()
                    for entries in run:
                        key = entries["key"]
                        val = entries["val"]
                        if kv.get(key) is not None:
                            raise Exception("Duplicated key for experiments")
                        else:
                            kv[key] = val
                    
                    def case_opt(key, f = None):
                        if kv.get(key) is None:
                            return None
                        else:
                            if f is None:
                                return kv[key]
                            else:
                                return f(kv[key])

                    if exclude is not None:
                        if exclude(kv):
                            continue

                    # Special handling for cilk
                    if kv['experiment'] == 'cilk_shootout':
                        benchcls = model.BenchmarkClass.high_parallelism
                        if kv['workstealing'] == 'cilk':
                            scheduler = model.Scheduler.cilk
                        elif kv['workstealing'] == 'abp':
                            scheduler = model.Scheduler.ne_abp
                        elif kv['workstealing'] == 'ywra':
                            scheduler = model.Scheduler.ne_ywra
                        elif kv['workstealing'] == 'cl':
                            continue # Skipping chaselev
                        else:
                            raise Exception("Unknown Cilk baseline scheduler variant")
                    else:
                        benchcls=model.BenchmarkClass[kv['experiment'].replace('-', '_')]
                        scheduler=model.Scheduler[kv['scheduler']]

                    # Constructing the objects
                    e = model.Experiments (
                        machine=machine,
                        benchmark=kv["benchmark"],
                        benchcls=benchcls,
                        mix_level=case_opt('mix_level_key'),
                        alpha=case_opt('TASKPARTS_ELASTIC_ALPHA'),
                        beta=case_opt('TASKPARTS_ELASTIC_BETA'),
                        numworkers=kv['TASKPARTS_NUM_WORKERS'],
                        bin=model.Binary[kv['binary']],
                        chaselev=case_opt('chaselev', lambda ch: model.Chaselev[ch]),
                        elastic=case_opt('elastic', lambda e: model.Elastic[e]),
                        exectime=kv['exectime'],
                        nb_fibers=case_opt('nb_fibers'),
                        nb_steals=case_opt('nb_steals'),
                        nivcsw=kv['nivcsw'],
                        nsignals=kv['nsignals'],
                        nvcsw=kv['nvcsw'],
                        scheduler=scheduler,
                        semaphore=case_opt('semaphore', lambda sem: model.SemImpl[sem]),
                        systime=kv['systime'],
                        total_idle_time=case_opt('total_idle_time'),
                        total_sleep_time=case_opt('total_sleep_time'),
                        total_time=case_opt( 'total_time' ),
                        total_work_time=case_opt( 'total_work_time' ),
                        usertime=kv['usertime'],
                        utilization=case_opt( 'utilization' ),
                        tp_numa_alloc_inter=(kv['TASKPARTS_NUMA_ALLOC_INTERLEAVED'] > 0),
                        tp_pin_worker=(kv['TASKPARTS_PIN_WORKER_THREADS'] > 0),
                        tp_binding=model.ResourceBinding[kv['TASKPARTS_RESOURCE_BINDING']],
                        file=filepath
                    )

                    session.add(e)
                session.commit()
            
if __name__ == "__main__":
    engine = setup_test()
    model.init(engine)
    Ing = Ingester(engine)
    Ing.ingest("json/aware.json", model.Machine.aware)
    Ing.ingest("json/aws.json", model.Machine.aws)
    Ing.ingest("json/gamora.json", model.Machine.gamora)

    # Test run -- lists all experiments
    with sqlalchemy.orm.Session(engine) as session:
        rslt = session.execute(sqlalchemy.select(model.Experiments))
        count = len(rslt.all())
        print(f"Found total {count} entries.")



