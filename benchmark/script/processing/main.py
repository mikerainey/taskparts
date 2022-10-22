import sqlalchemy
import ingest
import model
from latex import *
from model import Machine, Experiments, Scheduler
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
def setup(path=None, setup=False, echo=True) :
    if path is None:
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
    ing.ingest("json/aware.json", Machine.aware)
    ing.ingest("json/aws.json", Machine.aws)
    ing.ingest("json/gamora.json", Machine.gamora)

def three_way_compare(schedbase, sched1, sched2) :
    baseline = orm.aliased(Experiments, name="baseline")
    elastic  = orm.aliased(Experiments, name="elastic")
    elastic_spin = orm.aliased(Experiments, name="spin")

    # Labels are picked such that the mapper for the textabel maps 
    # the column to a texcolumn with identical name
    return (
        select(
            baseline.id.label("bid"),
            elastic.id.label("eid"),
            elastic.id.label("sid"),
            baseline.benchmark,
            baseline.machine,
            baseline.numworkers,
            baseline.scheduler,
            elastic.elastic.label("elastic"),
            elastic.scheduler,
            elastic_spin.scheduler,
            baseline.exectime.label("rt_baseline"),
            elastic.exectime.label("rt_spdup"),
            elastic_spin.exectime.label("rt_elastic"),
            baseline.total_time.label("burn_baseline"),
            elastic.total_time.label("burn_spdup"),
            elastic_spin.exectime.label("burn_elastic"),
        )
        .select_from(baseline, elastic, elastic_spin)
        .where(baseline.benchmark == elastic.benchmark)
        .where(baseline.machine == elastic.machine)
        .where(baseline.numworkers == elastic.numworkers)
        .where(baseline.benchmark == elastic_spin.benchmark)
        .where(baseline.machine == elastic_spin.machine)
        .where(baseline.numworkers == elastic_spin.numworkers)
        # Scheduler choices
        .where(baseline.scheduler == schedbase)
        .where(elastic.scheduler == sched1)
        .where(elastic_spin.scheduler == sched2)
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

def test_table_schema() :
    overheader = [ColSkip(3), ColumnLegend(width=3, text=r"\textbf{wall clock}"), ColumnLegend(width=3, text=r"\textbf{burn}")]
    header     = [
        ColSkip(1), ColumnLegend("#procs"), ColumnLegend("benchmark"),
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("spin"), 
        ColumnLegend("non-elastic"), ColumnLegend("elastic"), ColumnLegend("spin")]
    columns = [
        TexColString("benchset").setCommoning(),
        TexColString("numworkers").setCommoning(), 
        TexColString("benchmark").setAlign(Alignment.Left), 
        ColumnBar.Bar,
        TexColFloat("rt_baseline", ndigits=2),
        TexColFloat("rt_spdup", ndigits=2),
        TexColFloat("rt_elastic", ndigits=2),
        ColumnBar.Bar,
        TexColFloat("burn_baseline", ndigits=2),
        TexColFloat("burn_spdup", ndigits=2),
        TexColFloat("burn_elastic", ndigits=2),
    ]
    
    def mapper(row, name):
        if name == "benchset":
            if row.elastic is None:
                return "simpl"
            else:
                return row.elastic
        else: 
            return row[name]

    return (TexTableSchema(header, overheader, columns), mapper)


def main():
    # engine = setup(echo=False, path="data/data.db", setup=True)
    # engine = setup(echo=True, path="data/data.db")
    engine = setup(echo=False)

    # This ingests the json files 
    ingest_json(engine)

    # Test run -- lists all experiments
    with sqlalchemy.orm.Session(engine) as session:
        # rslt = session.execute(sqlalchemy.select(Experiments))
        # rslt = session.execute(sqlalchemy.select(Experiments))
        # print(len(rslt.all()))

        rslt1 = session.execute(three_way_compare(Scheduler.nonelastic, Scheduler.elastic, Scheduler.elastic_spin)).all()
        rslt2 = session.execute(three_way_compare(Scheduler.nonelastic, Scheduler.elastic2, Scheduler.elastic2_spin)).all()
        # for row in rslt1:
        #     print(row)
        # for row in rslt2:
        #     print(row)
        # count = len(all)
        # print(f"Found total {count} entries.")
    # print(rslt1[0].keys())

    schema, mapper = test_table_schema()
    print(generate_table(schema, rslt1 + rslt2, mapper))

if __name__ == "__main__":
	main()
