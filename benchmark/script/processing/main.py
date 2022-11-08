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

    #latest_results_folder = 'results-2022-11-05-15-30-37'
    latest_results_folder = 'merged-results-2022-11-08-01-19-45'
    
    aws("json/experiments/" + latest_results_folder + "/high_parallelism-results.json")
    aws("json/experiments/" + latest_results_folder + "/low_parallelism-results.json")
    aws("json/experiments/" + latest_results_folder + "/parallel_sequential_mix-results.json")
    #aws("json/experiments/" + latest_results_folder + "/multiprogrammed-results.json")
    
    # aws("json/experiments/results-2022-10-24-22-06-57/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-24-22-47-27/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/high_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/low_parallelism-results.json")
    # aws("json/experiments/results-2022-10-25-01-22-22/parallel_sequential_mix-results.json")
    # ing.ingest("json/aware.json", Machine.aware)
    # ing.ingest("json/aws.json", Machine.aws)
    # ing.ingest("json/gamora.json", Machine.gamora)


# This executes a three-way compare between one baseline and two possible other scheduler
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

# This executes two-way compare
def two_way_compare(schedbase, sched, table=Averaged) :
    baseline = orm.aliased(table, name="baseline")
    elastic  = orm.aliased(table, name="elastic")

    # Labels are picked such that the mapper for the textabel maps 
    # the column to a texcolumn with identical name
    return (
        select(
            # The following are all join conditions
            baseline.benchcls,
            baseline.machine,
            baseline.numworkers,
            baseline.scheduler,
            baseline.elastic.label("elastic"),
            baseline.benchmark,

            # runtime comparisons
            baseline.exectime_avg.label("rt_baseline"),
            elastic.exectime_avg.label("rt_elastic"),
            # ((elastic.exectime_avg - baseline.exectime_avg) / baseline.exectime_avg).label("rt_pctdiff"),

            # burn comparisons
            baseline.usertime_avg.label("burn_baseline"),
            elastic.usertime_avg.label("burn_elastic"),
            # ((elastic.usertime_avg - baseline.usertime_avg) / baseline.exectime_avg).label("burn_pctdiff"),

            # burn ratios
            (baseline.total_work_time_avg).label("work_baseline"),
            (elastic.total_work_time_avg).label("work_elastic"),
            # (baseline.usertime_avg / baseline.total_work_time_avg).label("br_baseline"),
            # (elastic.usertime_avg / elastic.total_work_time_avg).label("br_elastic"),
        )
        .select_from(baseline, elastic)
        .where(baseline.benchcls == elastic.benchcls)
        .where(baseline.benchmark == elastic.benchmark)
        .where(baseline.machine == elastic.machine)
        .where(baseline.numworkers == elastic.numworkers)
        # Scheduler choices
        .where(baseline.scheduler == schedbase)
        .where(elastic.scheduler == sched)
        .order_by(baseline.benchcls.asc())
        .order_by(baseline.numworkers.asc())
        .order_by(baseline.machine)
        .order_by(baseline.benchmark)
    )

def main_table_schema() :
    overheader = [ColSkip(2), 
        ColumnLegend(width=3, text=r"\textbf{Runtime (s)}"), ColumnLegend(width=3, text=r"\textbf{Burn (s)}"), 
        ColumnLegend(width=2, text=r"\textbf{Work (s)}"), ColumnLegend(width=2, text=r"\textbf{Burn ratio}")]
    header     = [
        ColSkip(2), 
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"), ColumnLegend(r"$\Delta_T$"), 
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"), ColumnLegend(r"$\Delta_B$"), 
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"),
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"), 
        ]
    columns = [
        TexColString("benchcls").setCommoning(),
        TexColString("benchmark").setAlign(Alignment.Left), 
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColFloat("rt_elastic", ndigits=2),
        TexColPercentage("rt_pctdiff", ndigits=2).setAlign(Alignment.Left),
        ColumnBar.Bar,
        TexColFloat("burn_baseline", ndigits=2),
        TexColFloat("burn_elastic", ndigits=2),
        TexColPercentage("burn_pctdiff", ndigits=2).setAlign(Alignment.Left),
        ColumnBar.Bar,
        TexColFloat("work_baseline", ndigits=2),
        TexColFloat("work_elastic", ndigits=2),
        ColumnBar.Bar,
        TexColMultiple("br_baseline", ndigits=2),
        TexColMultiple("br_elastic", ndigits=2),
    ]
    
    # We compute the derived columns here
    def mapper(row, name):
        def safe_div(x, y):
            try:
                return x / y 
            except ZeroDivisionError:
                print("Warning: Division-by-zero in ratio computation", file=stderr)
                return float('nan')

        if name == "rt_pctdiff":
            return safe_div(row.rt_elastic - row.rt_baseline, row.rt_baseline)
        elif name == "burn_pctdiff":
            return safe_div(row.burn_elastic - row.burn_baseline, row.burn_baseline)
        elif name == "br_baseline":
            return safe_div(row.burn_baseline, row.work_baseline)
        elif name == "br_elastic":
            return safe_div(row.burn_elastic, row.work_elastic)
        else:
            return row[name]

    return (TexTableSchema(header, overheader, columns), mapper)

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
        TexColFloat("rt_pctdiff", ndigits=2),
#        TexColFloat("rt_elastic", ndigits=2),
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
    overheader = [ColSkip(2), ColumnLegend(width=3, text=r"\textbf{Runtime (w)}"), ColumnLegend(width=3, text=r"\textbf{Burn (s)}")]
    header     = [
        ColumnLegend("Kind"), 
        ColumnLegend("Benchmark"), 
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"), ColumnLegend("Spin"), 
        ColumnLegend("Non-elastic"), ColumnLegend("Elastic"), ColumnLegend("Spin")]
    columns = [
        # TexColInteger("numworkers").setCommoning(), 
        TexColEnum("benchcls").setCommoning(),
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

    def mapper(row, name):
        return row[name]

    return (TexTableSchema(header, overheader, columns), mapper)

def main():
    # engine = setup(echo=True, remote=True, setup=True)
    # engine = setup(echo=True, path="data/data.db")
    engine = setup(echo=False)

    # This ingests the json files 
    ingest_json(engine)

    maintbl_tex = 'tex/maintbl.tex'
    multiprog_tex = 'tex/multiprog.tex'
    appendix_tex = 'tex/appendix.tex'

    with sqlalchemy.orm.Session(engine) as session:

        # Main table
        rslt = session.execute(two_way_compare(Scheduler.nonelastic, Scheduler.elastic2)).all()
        with open(maintbl_tex, 'w') as fout:
            schema, mapper = main_table_schema()
            print(doc_frame(generate_table(schema, rslt, mapper)), file=fout)

        # Appendix table
        rslt = session.execute(three_way_compare(Scheduler.nonelastic, Scheduler.elastic2, Scheduler.elastic2_spin)).all()
        with open(appendix_tex, 'w') as fout:
            schema, mapper = appendix_table_schema()
            print(doc_frame(generate_table(schema, rslt, mapper)), file=fout)

        print(f'Done. Tex results written to "{maintbl_tex}" and "{appendix_tex} respectively"')

    # schema, mapper = test_table_schema()

    # maintbl_tex = 'tex/maintbl.tex'
    # multiprog_tex = 'tex/multiprog.tex'

    # with open(maintbl_tex, 'w') as fout:
    #     print(doc_frame(generate_table(schema, rslt2, mapper)), file=fout)

    # schema, mapper = multiprogrammed_schema()
    # with open(multiprog_tex, 'w') as fout:
    #     print(doc_frame(generate_table(schema, rsltm1, mapper)), file=fout)

    # print(f'Done. Tex results written to "{maintbl_tex}" and "{multiprog_tex} respectively"')

if __name__ == "__main__":
	main()
