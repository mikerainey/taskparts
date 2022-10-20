import enum
from multiprocessing import Semaphore
from tkinter.tix import COLUMN
from tokenize import Double
from sqlalchemy import *
from sqlalchemy.orm import *

Base = declarative_base()

@enum.unique
class Machine(enum.Enum):
    aware = enum.auto()
    aws = enum.auto()
    gamora = enum.auto()

@enum.unique
class Binary(enum.Enum):
    sta = enum.auto()

@enum.unique
class Chaselev(enum.Enum):
    elastic  = enum.auto()

@enum.unique
class Elastic(enum.Enum):
    surplus = enum.auto()
    surplus2 = enum.auto()

@enum.unique
class Scheduler(enum.Enum):
    nonelastic = enum.auto()
    elastic = enum.auto()
    elastic2 = enum.auto()
    elastic_spin = enum.auto()
    elastic2_spin = enum.auto()

@enum.unique
class SemImpl(enum.Enum):
    spin = enum.auto()

class Experiments(Base):
    __tablename__ = "experiements"

    id               = Column(Integer, primary_key=True)
    machine          = Column(Enum(Machine))
    numworkers       = Column(Integer)
    benchmark        = Column(String)
    bin              = Column(Enum(Binary)) # Renamed to avoid conflict with type
    chaselev         = Column(Enum(Chaselev))
    elastic          = Column(Enum(Elastic))
    exectime         = Column(Float)
    maxrss           = Column(Integer)
    nb_steals        = Column(Integer)
    nb_fibers        = Column(Integer)
    nivcsw           = Column(Integer)
    nsignals         = Column(Integer)
    nvcsw            = Column(Integer)
    scheduler        = Column(Enum(Scheduler))
    semaphore        = Column(Enum(SemImpl))
    systime          = Column(Float)
    total_idle_time  = Column(Float)
    total_sleep_time = Column(Float)
    total_time       = Column(Float)
    total_work_time  = Column(Float)
    usertime         = Column(Float)
    utilization      = Column(Float)

# This creates all tabels with the given engine
# Should only be called for the initial ingest
def init(engine):
    Base.metadata.create_all(engine)