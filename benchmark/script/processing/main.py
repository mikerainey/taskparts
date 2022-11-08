from sys import stderr
import sqlalchemy
import ingest
import pathlib
import model
from latex import *
from model import Averaged, Machine, Experiments, Scheduler
from sqlalchemy import *
import sqlalchemy.orm as orm

# Sets up a connection to a database.
# The db can either reside in-memory (for generating tables for the paper) 
# or it can sit on a disk (for separate analysis, or visualization).
#
# If the DB resides in memory then it will preform the initialization.
# If the DB resides on disk then by default it will be used as is, assuming 
# the database has already been setup some time. This can be overriden by 
# setting setup=True
#
# The function returns the created engine.
def setup(path=None, setup=False, echo=True, remote=False) :
    if remote:
        print("Connecting to remote db.", stderr)
        # (!!) You really should not put username/password in the code
        # However we are using free tier services and on a ddl crunch.
        user = "pop"
        passwd = "scheduler"
        address = "experiments.cz6zabb0vywb.us-east-1.rds.amazonaws.com"
        engine = sqlalchemy.create_engine(f'postgresql+psycopg2://{user}:{passwd}@{address}/postgres')
        if setup:
            model.init(engine)
        return engine
    elif path is None :
        engine = sqlalchemy.create_engine("sqlite+pysqlite:///:memory:", echo=echo, future=True)
        model.init(engine)
        return engine
    else:
        engine = sqlalchemy.create_engine(f"sqlite+pysqlite:///{path}", echo=echo, future=True)
        if setup:
            model.init(engine)
        return engine

def ingest_json(engine):
    ing = ingest.Ingester(engine)
    
    def exclude(kv):
        return False #kv['benchmark'] == 'suffixarray'

    def aws(path):
        ing.ingest(path, Machine.aws, exclude=exclude)

    latest_results_folder = 'results-2022-11-05-15-30-37'
    aws("json/experiments/" + latest_results_folder + "/high_parallelism-results.json")
    aws("json/experiments/" + latest_results_folder + "/low_parallelism-results.json")
    aws("json/experiments/" + latest_results_folder + "/parallel_sequential_mix-results.json")
    aws("json/experiments/" + latest_results_folder + "/multiprogrammed-results.json")
    
    # aws("json/experiments/results-2022-10-24-22-06-57/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-24-22-47-27/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/low_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/parallel_sequential_mix-results.json")
    # ing.ingest("json/aware.json", Machine.aware)
    # ing.ingest("json/aws.json", Machine.aws)
    # ing.ingest("json/gamora.json", Machine.gamora)

def three_way_compare(schedbase, sched1, sched2, table=Averaged) :
    baseline = orm.aliased(table, name="baseline")
    elastic  = orm.aliased(table, name="elastic")
    elastic_spin = orm.aliased(table, name="spin")

    # Labels are picked such that the mapper for the textabel maps 
    # the column to a texcolumn with identical name
    return (
        select(
            # baseline.id.label("bid"),
            # elastic.id.label("eid"),
            # elastic_spin.id.label("sid"),
            baseline.benchcls,
            baseline.benchmark,
            baseline.machine,
            baseline.numworkers,
            baseline.scheduler,
            elastic.elastic.label("elastic"),
            elastic.scheduler,
            elastic_spin.scheduler,
            baseline.exectime_avg.label("rt_baseline"),
            elastic.exectime_avg.label("rt_elastic"),
            elastic_spin.exectime_avg.label("rt_elastic2"),
            baseline.usertime_avg.label("burn_baseline"),
            elastic.usertime_avg.label("burn_elastic"),
            elastic_spin.usertime_avg.label("burn_elastic2"),
        )
        .select_from(baseline, elastic, elastic_spin)
        .where(baseline.benchcls == elastic.benchcls)
        .where(baseline.benchmark == elastic.benchmark)
        .where(baseline.machine == elastic.machine)
        .where(baseline.numworkers == elastic.numworkers)
        .where(baseline.benchcls == elastic_spin.benchcls)
        .where(baseline.benchmark == elastic_spin.benchmark)
        .where(baseline.machine == elastic_spin.machine)
        .where(baseline.numworkers == elastic_spin.numworkers)
        # Scheduler choices
        .where(baseline.scheduler == schedbase)
        .where(elastic.scheduler == sched1)
        .where(elastic_spin.scheduler == sched2)
        .order_by(baseline.benchcls.asc())
        .order_by(baseline.numworkers.asc())
        .order_by(baseline.machine)
        .order_by(baseline.benchmark)
    )

def main_table_schema() :
    overheader = [ColSkip(3), ColumnLegend(width=3, text=r"\textbf{wall clock}"), ColumnLegend(width=2, text=r"\textbf{burn}")]
    header     = [ColSkip(3), ColumnLegend("non-elastic"), ColumnLegend("speedup"), ColumnLegend("elastic"), ColumnLegend("non-elastic"), ColumnLegend("elastic")]
    columns = [
        TexColString("benchset").setCommoning(),
        TexColString("benchmark").setAlign(Alignment.Left), 
        TexColInteger("numworkers").setCommoning(), 
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColMultiple("rt_spdup", ndigits=2).setAlign(Alignment.Left),
        TexColPercentage("rt_elastic", ndigits=2).setAlign(Alignment.Left),
        ColumnBar.Bar,
        TexColFloat("burn_nonelastic", ndigits=2),
        TexColPercentage("burn_elastic", ndigits=2).setAlign(Alignment.Left),
    ]
    return TexTableSchema(header, overheader, columns)

def multiprogrammed_schema() :
    overheader = [ColSkip(3), ColumnLegend(width=3, text=r"\textbf{wall clock}"), ColumnLegend(width=3, text=r"\textbf{burn}")]
    header     = [
        ColumnLegend("class"), 
        # ColumnLegend("scheduler"), 
        ColumnLegend("\\#procs"), 
        ColumnLegend("benchmark"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("multiprog"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("multiprog")]
    columns = [
        TexColEnum("benchcls").setCommoning(),
        TexColInteger("numworkers").setCommoning(), 
        TexColString("benchmark").setAlign(Alignment.Left), 
        # TexColString("scheduler"),
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColFloat("rt_elastic", ndigits=2),
        TexColFloat("rt_elastic2", ndigits=2),
        ColumnBar.Bar,
        TexColFloat("burn_baseline", ndigits=2),
        TexColFloat("burn_elastic", ndigits=2),
        TexColFloat("burn_elastic2", ndigits=2),
    ]
    
    def mapper(row, name):
        # if name == "benchset":
        if name == "scheduler":
            if row.elastic is None:
                return "simpl"
            else:
                return row.elastic.name
        else: 
            return row[name]

    return (TexTableSchema(header, overheader, columns), mapper)

def test_table_schema() :
    overheader = [ColSkip(2), ColumnLegend(width=2, text=r"\textbf{wall clock}"), ColumnLegend(width=2, text=r"\textbf{burn}")]
    header     = [
#        ColumnLegend("\\#procs123"), 
#        ColumnLegend("scheduler"), 
        ColumnLegend(""), 
        ColumnLegend("benchmark"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic")]
    columns = [
#        TexColInteger("numworkers").setCommoning(), 
#        TexColString("scheduler").setCommoning(),
        TexColEnum("benchcls").setCommoning(),
        # TexColString("machine").setCommoning(), 
        TexColString("benchmark").setAlign(Alignment.Left), 
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColFloat("rt_elastic", ndigits=2),
#        TexColFloat("rt_elastic2", ndigits=2),
        ColumnBar.Bar,
        TexColFloat("burn_baseline", ndigits=2),
        TexColFloat("burn_elastic", ndigits=2),
#        TexColFloat("burn_elastic2", ndigits=2),
    ]

    def mapper(row, name):
        # if name == "benchset":
        if name == "scheduler":
            if row.elastic is None:
                return "simpl"
            else:
                return row.elastic
        else: 
            return row[name]

    return (TexTableSchema(header, overheader, columns), mapper)

def appendix_table_schema() :
    overheader = [ColSkip(4), ColumnLegend(width=3, text=r"\textbf{wall clock}"), ColumnLegend(width=3, text=r"\textbf{burn}")]
    header     = [
        ColumnLegend("\\#procs123"), 
        ColumnLegend("scheduler"), 
        ColumnLegend("class"), 
        ColumnLegend("benchmark"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("spin"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("spin")]
    columns = [
        TexColInteger("numworkers").setCommoning(), 
        TexColString("scheduler").setCommoning(),
        TexColEnum("benchcls").setCommoning(),
        # TexColString("machine").setCommoning(), 
        TexColString("benchmark").setAlign(Alignment.Left), 
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColFloat("rt_elastic", ndigits=2),
        TexColFloat("rt_elastic2", ndigits=2),
        ColumnBar.Bar,
        TexColFloat("burn_baseline", ndigits=2),
        TexColFloat("burn_elastic", ndigits=2),
        TexColFloat("burn_elastic2", ndigits=2),
    ]

def main():
    # engine = setup(echo=True, remote=True, setup=True)
    # engine = setup(echo=True, path="data/data.db")
    engine = setup(echo=False)

    # This ingests the json files 
    ingest_json(engine)

    # Test run -- lists all experiments
    with sqlalchemy.orm.Session(engine) as session:
        # rslt = session.execute(sqlalchemy.select(Experiments))
        # rslt = session.execute(sqlalchemy.select(Experiments))
        

        # rslt = session.execute(sqlalchemy.select(Experiments))
        # print(len(rslt.all()))
        #q1 = three_way_compare(Scheduler.nonelastic, Scheduler.elastic, Scheduler.elastic_spin)
        # print(q1)
        # print("")
        q2 = three_way_compare(Scheduler.nonelastic, Scheduler.elastic2, Scheduler.elastic2_spin)
        # print(q2)
        # print("")

        #rslt1 = session.execute(q1).all()
        rslt2 = session.execute(q2).all()
        # for row in rslt1:
        #     print(row)
        # for row in rslt2:
        #     print(row)
        # count = len(all)
        # print(f"Found total {count} entries.")

        m1 = three_way_compare(Scheduler.nonelastic, Scheduler.elastic2, Scheduler.multiprogrammed)
        rsltm1 = session.execute(m1).all()
    # print(rslt1[0].keys())

    schema, mapper = test_table_schema()

    maintbl_tex = 'tex/maintbl.tex'
    multiprog_tex = 'tex/multiprog.tex'

    with open(maintbl_tex, 'w') as fout:
        print(doc_frame(generate_table(schema, rslt2, mapper)), file=fout)

    schema, mapper = multiprogrammed_schema()
    with open(multiprog_tex, 'w') as fout:
        print(doc_frame(generate_table(schema, rsltm1, mapper)), file=fout)

    print(f'Done. Tex results written to "{maintbl_tex}" and "{multiprog_tex} respectively"')

if __name__ == "__main__":
	main()
