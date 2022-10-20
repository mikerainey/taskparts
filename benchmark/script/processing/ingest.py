import sqlalchemy 
import model
import json

def setup_test() :
    engine = sqlalchemy.create_engine("sqlite+pysqlite:///:memory:", echo=True, future=True)
    return engine

class Ingester:
    def __init__(self, engine) -> None:
        self.engine = engine
        pass

    def ingest(self, filepath, machine : model.Machine):
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
                    
                    def case_opt(key, f):
                        if kv.get(key) is None:
                            return None
                        else:
                            return f(kv[key])

                    # Constructing the objects
                    e = model.Experiments (
                        machine=machine,
                        benchmark=kv["benchmark"],
                        numworkers=kv['TASKPARTS_NUM_WORKERS'],
                        bin=model.Binary[kv['binary']],
                        chaselev=case_opt('chaselev', lambda ch: model.Chaselev[ch]),
                        elastic=case_opt('elastic', lambda e: model.Elastic[e]),
                        exectime=kv['exectime'],
                        nb_fibers=kv['nb_fibers'],
                        nb_steals=kv['nb_steals'],
                        nivcsw=kv['nivcsw'],
                        nsignals=kv['nsignals'],
                        nvcsw=kv['nvcsw'],
                        scheduler=model.Scheduler[kv['scheduler']],
                        semaphore=case_opt('semaphore', lambda sem: model.SemImpl[sem]),
                        systime=kv['systime'],
                        total_idle_time=kv['total_idle_time'],
                        total_sleep_time=kv['total_sleep_time'],
                        total_time=kv['total_time'],
                        total_work_time=kv['total_work_time'],
                        usertime=kv['usertime'],
                        utilization=kv['utilization'],
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



