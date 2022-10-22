import enum
from sqlalchemy import *
from sqlalchemy.orm import *
import view

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

    # def __repr__(self):
    #     return f"Run({self.id!r}, {self.machine!r}, {self.scheduler!r}, {self.numworkers!r}, {self.utilization!r}"

# Baseline view created upon the experiments set
class Baseline(Base):
    __table__ = view.view (
        "baseline",
        Base.metadata,
        select(
            Experiments.id.label("id"),
            Experiments.machine.label("machine"),
            Experiments.benchmark.label("benchmark"),
            Experiments.scheduler.label("scheduler"),
            Experiments.numworkers.label("numworkers"),
            Experiments.exectime.label("exectime")
        )
        .where(Experiments.scheduler == "nonelastic")
    )

# This creates all tabels with the given engine
# Should only be called for the initial ingest
def init(engine):
    Base.metadata.create_all(engine)

