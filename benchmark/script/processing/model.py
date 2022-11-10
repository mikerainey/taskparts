import enum
from tokenize import group
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

    def __str__(self) -> str:
        if self.value == self.__class__.surplus.value :
            return "surplus"
        elif self.value == self.__class__.surplus2.value :
            return "surplus2"
        assert False


@enum.unique
class ResourceBinding(enum.Enum):
    by_core = enum.auto()

@enum.unique
class BenchmarkClass(enum.Enum):
    high_parallelism = enum.auto()
    low_parallelism = enum.auto()
    parallel_sequential_mix = enum.auto()
    multiprogrammed = enum.auto()

    def __str__(self) -> str:
        if self.name == "high_parallelism":
            return "high"
        elif self.name == "low_parallelism":
            return "low"
        elif self.name == "parallel_sequential_mix":
            return "mixed"
        elif self.name == "multiprogrammed":
            return "multiprogrammed"
        else:
            raise Exception('Unknown benchcls')

@enum.unique
class MixLevel(enum.Enum):
    low = enum.auto()
    med = enum.auto()
    large = enum.auto()

@enum.unique
class Scheduler(enum.Enum):
    nonelastic = enum.auto()
    elastic = enum.auto()
    elastic2 = enum.auto()
    elastic_spin = enum.auto()
    elastic2_spin = enum.auto()
    multiprogrammed = enum.auto()
    cilk = enum.auto()
    ne_abp = enum.auto() # Baseline only used in Cilk shootout
    ne_ywra = enum.auto() # Baseline only used in Cilk shootout

@enum.unique
class SemImpl(enum.Enum):
    spin = enum.auto()

class Experiments(Base):
    __tablename__ = "experiements"

    id               = Column(Integer, primary_key=True)
    machine          = Column(Enum(Machine))
    numworkers       = Column(Integer)
    benchmark        = Column(String)
    benchcls         = Column(Enum(BenchmarkClass))
    mix_level        = Column(Enum(MixLevel))
    alpha            = Column(Integer)
    beta             = Column(Integer)
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
    
    tp_numa_alloc_inter = Column(Boolean)
    tp_pin_worker       = Column(Boolean)
    tp_binding          = Column(Enum(ResourceBinding))

    file = Column(String)
    
    # def __repr__(self):
    #     return f"Run({self.id!r}, {self.machine!r}, {self.scheduler!r}, {self.numworkers!r}, {self.utilization!r}"


# Baseline view created upon the experiments set
class Averaged(Base):
    def grouping(c):
        return [
            c.machine,
            c.numworkers,
            c.benchcls,
            c.mix_level,
            c.alpha,
            c.beta,
            c.benchmark,
            c.scheduler,
            c.semaphore,
            c.elastic,
        ]

    _avg_stmt = (
        select(grouping(Experiments) +
        [
            func.avg(Experiments.exectime).label("exectime_avg"),
            func.avg(Experiments.systime).label("systime_avg"),
            func.avg(Experiments.usertime).label("usertime_avg"),
            func.avg(Experiments.total_time).label("total_time_avg"),
            func.avg(Experiments.total_idle_time).label("total_idle_time_avg"),
            func.avg(Experiments.total_work_time).label("total_work_time_avg"),
            func.avg(Experiments.total_sleep_time).label("total_sleep_time_avg"),
            func.avg(Experiments.utilization).label("utilization_avg"),
        ])
        .select_from(Experiments)
        .group_by(*grouping(Experiments))
    )

    __table__ = view.view (
        "averaged",
        Base.metadata, _avg_stmt
    )

    __mapper_args__ = {
        "primary_key" : grouping(__table__.c)
    }

# This creates all tabels with the given engine
# Should only be called for the initial ingest
def init(engine):
    Base.metadata.drop_all(engine)
    Base.metadata.create_all(engine)

