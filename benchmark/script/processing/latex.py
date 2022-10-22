# This file dumps the result set of a query into latex tables according to a "Schema"
from dataclasses import dataclass
import enum

@enum.unique
class Alignment(enum.Enum):
	Left = enum.auto()
	Center = enum.auto()
	Right = enum.auto()

@enum.unique
class ColumnBar(enum.Enum):
	Bar = enum.auto()
	DoubleBar = enum.auto()

@dataclass
class ColumnLegend:
	text  : str
	width : int = 1
	fmt   : str = "c"

def ColSkip(width):
	return ColumnLegend(width=width, fmt="c", text=" ")

@dataclass
class TexColumn:
	name      : str            # Name of the TexColumn
	commoning : bool = False   # Requires vertical 'commoning', i.e. combine rows with same values
	to_string : str  = None    # Custom to_string function
	alignment : Alignment = Alignment.Center
	math      : bool = False

	def setAlign(self, align):
		self.alignment = align
		return self

	def setCommoning(self, commoning=True):
		self.commoning = commoning
		return self

# Useful columns
def TexColString(name):
	return TexColumn(name, to_string=lambda x : x)

def TexColMath(name):
	return TexColumn(name, to_string=lambda x : '''\ensuremath{%s}''' % x)

# By default rounds to nearest 0.01
def TexColFloat(name, ndigits=2):
	return TexColumn(name, to_string=lambda x : f"{round(x, ndigits=ndigits)}")

# Renders a floating number (without multiplication) to nearest 0.1%
def TexColPercentage(name, ndigits=1):
	return TexColumn(name, to_string=lambda x : f"{round(x * 100, ndigits=ndigits)}\\%")

# Renders a floating number to a multiple (to nearest 0.1x)
def TexColMultiple(name, ndigits=1):
	return TexColumn(name, to_string=lambda x : f"{round(x, ndigits=ndigits)}x")

def _col_to_shape(col) : 
	if isinstance(col, ColumnBar):
		if col == ColumnBar.Bar:
			return "|"
		elif col == ColumnBar.DoubleBar:
			return "||"
		else:
			raise Exception()
	elif isinstance(col, TexColumn):
		if col.alignment == Alignment.Left:
			return "l"
		elif col.alignment == Alignment.Center:
			return "c"
		elif col.alignment == Alignment.Right:
			return "r"
		else:
			raise Exception()
	else:
		raise Exception()

@dataclass
class TexTableSchema: 
	width : int          # width  of the table, i.e number of columns
	header : list        # header of the table,
	overheader : list    # An optional header of the header
	columns : list       # A list of TexColumn
	shape : list         # Shape specifier string

        # Validates width constraints
	def __init__(self, header, overheader, columns):
                # Splits out actual columns
		actual_columns = [c for c in columns if isinstance(c, TexColumn)]
		shape = [_col_to_shape(c) for c in columns]

		width=len(actual_columns)
		# Make sure column legend matches up
		assert sum([h.width for h in header]) == width
		if overheader is not None:
			assert sum([h.width for h in overheader]) == width

		self.width = width
		self.header = header
		self.overheader = overheader
		self.columns = actual_columns
		self.shape = shape

def _legend_to_str(leg : ColumnLegend):
	if leg.width > 1:
		return '\\multicolumn{%d}{%s}{%s}' % (leg.width, leg.fmt, leg.text)
	else:
		return leg.text

# Generates a tex table with schema specified by 'schema', 
# Rows are coming from 'rslt'
# The data contained in 'row' are mapped to columsn of the table using the mapper
def generate_table(schema: TexTableSchema, rslt, mapper):
	lines = [] # output buffer

	## Table header
	lines.append(r"\begin{tabular}{" + "".join(schema.shape) + r"}")
	lines.append(r"\toprule")

	if schema.overheader is not None:
		lines.append(" & ".join([_legend_to_str(leg) for leg in schema.overheader]))

	lines.append(" & ".join([_legend_to_str(leg) for leg in schema.header]))
	lines.append(r"\midrule")


	# We will start emitting rows
	nrows = len(rslt)

	rows_str = []
	# We will emit the rows in reverse order
	# For every commoning column we will keep a counter, 
	# counting how many identical rows we have met


	lines.append(r"\bottomrule")
	lines.append(r"\end{tabular}")
	return "\n".join(lines)
	
	