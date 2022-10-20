import sqlalchemy
import ingest
import model
from model import Machine, Experiments
from sqlalchemy import *
from sqlalchemy import *

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

def ingest(engine):
    ing = ingest.Ingester(engine)
    ing.ingest("json/aware.json", Machine.aware)
    ing.ingest("json/aws.json", Machine.aws)
    ing.ingest("json/gamora.json", Machine.gamora)

def main():
    # engine = setup(echo=False, path="data/data.db", setup=True)
    engine = setup(echo=True, path="data/data.db")

    # This ingests the json files 
    # ingest(engine)

    # Test run -- lists all experiments
    with sqlalchemy.orm.Session(engine) as session:
        rslt = session.execute(sqlalchemy.select(Experiments))
        count = len(rslt.all())
        print(f"Found total {count} entries.")

if __name__ == "__main__":
	main()
