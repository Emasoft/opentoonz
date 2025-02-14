

#ifndef TPROPERTY_INCLUDED
#define TPROPERTY_INCLUDED

#include "tconvert.h"

#undef DVAPI
#undef DVVAR

#ifdef TNZCORE_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

template <class T>
class TRangeProperty;

typedef TRangeProperty<int> TIntProperty;
typedef TRangeProperty<double> TDoubleProperty;

class DVAPI TBoolProperty;
class DVAPI TStringProperty;
class DVAPI TEnumProperty;
class DVAPI TDoublePairProperty;
class DVAPI TIntPairProperty;
class DVAPI TStyleIndexProperty;
class DVAPI TPointerProperty;

class TIStream;
class TOStream;

//---------------------------------------------------------

class DVAPI TProperty
{
public:
	class Visitor
	{
	public:
		virtual void visit(TDoubleProperty *p) = 0;
		virtual void visit(TIntProperty *p) = 0;
		virtual void visit(TBoolProperty *p) = 0;
		virtual void visit(TStringProperty *p) = 0;
		virtual void visit(TEnumProperty *p) = 0;
		virtual void visit(TDoublePairProperty *p) = 0;
		virtual void visit(TIntPairProperty *p) = 0;
		virtual void visit(TStyleIndexProperty *p) = 0;
		virtual void visit(TPointerProperty *p) = 0;
		virtual ~Visitor() {}
	};

	class Listener
	{
	public:
		virtual void onPropertyChanged() = 0;
		virtual ~Listener() {}
	};

	// eccezioni
	class TypeError
	{
	};
	class RangeError
	{
	};

	TProperty(std::string name)
		: m_name(name)
	{
		m_qstringName = QString::fromStdString(name);
	}

	virtual ~TProperty() {}

	virtual TProperty *clone() const = 0;

	// Used only for translation in Qt
	QString getQStringName() const { return m_qstringName; }
	void setQStringName(const QString &str) { m_qstringName = str; }

	std::string getName() const { return m_name; }
	virtual std::string getValueAsString() = 0;

	virtual void accept(Visitor &v) = 0;

	void addListener(Listener *listener);
	void removeListener(Listener *listener);
	void notifyListeners() const;

	std::string getId() const { return m_id; }
	void setId(std::string id) { m_id = id; }

private:
	std::string m_name;
	QString m_qstringName;
	std::string m_id;
	std::vector<Listener *> m_listeners;
};

//---------------------------------------------------------

template <class T>
class TRangeProperty : public TProperty
{

public:
	typedef std::pair<T, T> Range;

	TRangeProperty(
		std::string name,
		T minValue, T maxValue,
		T value, bool isMaxRangeLimited = true)
		: TProperty(name), m_range(minValue, maxValue), m_value(minValue), m_isMaxRangeLimited(isMaxRangeLimited)
	{
		setValue(value);
	}

	TProperty *clone() const
	{
		return new TRangeProperty<T>(*this);
	}

	Range getRange() const { return m_range; }

	void setValue(T v, bool cropEnabled = false)
	{
		if (cropEnabled && m_isMaxRangeLimited)
			v = tcrop(v, m_range.first, m_range.second);
		if (cropEnabled && !m_isMaxRangeLimited)
			v = v < m_range.first ? m_range.first : v;
		if (v < m_range.first || (v > m_range.second && m_isMaxRangeLimited))
			throw RangeError();
		m_value = v;
	}

	T getValue() const
	{
		return m_value;
	}

	std::string getValueAsString()
	{
		return toString(m_value);
	}

	void accept(Visitor &v) { v.visit(this); }

	bool isMaxRangeLimited() const { return m_isMaxRangeLimited; }

private:
	Range m_range;
	T m_value;
	bool m_isMaxRangeLimited;
};

//---------------------------------------------------------
#ifdef _WIN32
template class DVAPI TRangeProperty<int>;
template class DVAPI TRangeProperty<double>;
#endif
//---------------------------------------------------------

class TDoublePairProperty : public TProperty
{

public:
	typedef std::pair<double, double> Range;
	typedef std::pair<double, double> Value;

	TDoublePairProperty(
		std::string name,
		double minValue, double maxValue,
		double v0, double v1,
		bool isMaxRangeLimited = true)
		: TProperty(name), m_range(Range(minValue, maxValue)), m_isMaxRangeLimited(isMaxRangeLimited)
	{
		setValue(Value(v0, v1));
	}

	TProperty *clone() const
	{
		return new TDoublePairProperty(*this);
	}

	Range getRange() const { return m_range; }

	bool isMaxRangeLimited() const { return m_isMaxRangeLimited; }

	void setValue(const Value &value)
	{
		if (value.first < m_range.first || (m_isMaxRangeLimited && value.first > m_range.second) ||
			value.second < m_range.first || (m_isMaxRangeLimited && value.second > m_range.second))
			throw RangeError();
		m_value = value;
	}
	Value getValue() const
	{
		return m_value;
	}
	std::string getValueAsString()
	{
		return toString(m_value.first) + "," + toString(m_value.second);
	}
	void accept(Visitor &v) { v.visit(this); };

private:
	Range m_range;
	Value m_value;
	bool m_isMaxRangeLimited;
};

//---------------------------------------------------------

class TIntPairProperty : public TProperty
{

public:
	typedef std::pair<int, int> Range;
	typedef std::pair<int, int> Value;

	TIntPairProperty(
		std::string name,
		int minValue, int maxValue,
		int v0, int v1,
		bool isMaxRangeLimited = true)
		: TProperty(name), m_range(minValue, maxValue), m_isMaxRangeLimited(isMaxRangeLimited)
	{
		setValue(Value(v0, v1));
	}

	TProperty *clone() const
	{
		return new TIntPairProperty(*this);
	}

	Range getRange() const { return m_range; }

	bool isMaxRangeLimited() const { return m_isMaxRangeLimited; }

	void setValue(const Value &value)
	{
		if (value.first < m_range.first || (m_isMaxRangeLimited && value.first > m_range.second) ||
			value.second < m_range.first || (m_isMaxRangeLimited && value.second > m_range.second))
			throw RangeError();
		m_value = value;
	}
	Value getValue() const
	{
		return m_value;
	}
	std::string getValueAsString()
	{
		return toString(m_value.first) + "," + toString(m_value.second);
	}
	void accept(Visitor &v) { v.visit(this); };

private:
	Range m_range;
	Value m_value;
	bool m_isMaxRangeLimited;
};

//---------------------------------------------------------

class DVAPI TBoolProperty : public TProperty
{
public:
	TBoolProperty(std::string name, bool value)
		: TProperty(name), m_value(value)
	{
	}

	TProperty *clone() const { return new TBoolProperty(*this); }

	void setValue(bool v) { m_value = v; }
	bool getValue() const { return m_value; }
	std::string getValueAsString()
	{
		return toString(m_value);
	}
	void accept(Visitor &v) { v.visit(this); };

private:
	bool m_value;
};

//---------------------------------------------------------

class DVAPI TStringProperty : public TProperty
{
public:
	TStringProperty(std::string name, std::wstring value)
		: TProperty(name), m_value(value)
	{
	}

	TProperty *clone() const { return new TStringProperty(*this); }

	void setValue(std::wstring v) { m_value = v; }
	std::wstring getValue() const { return m_value; }
	std::string getValueAsString()
	{
		return toString(m_value);
	}
	void accept(Visitor &v) { v.visit(this); };

private:
	std::wstring m_value;
};

//---------------------------------------------------------

class DVAPI TStyleIndexProperty : public TProperty
{
public:
	TStyleIndexProperty(std::string name, std::wstring value)
		: TProperty(name), m_value(value)
	{
	}

	TProperty *clone() const { return new TStyleIndexProperty(*this); }

	void setValue(std::wstring v) { m_value = v; }
	std::wstring getValue() const { return m_value; }

	std::string getValueAsString()
	{
		return toString(m_value);
	}

	void accept(Visitor &v) { v.visit(this); };

private:
	std::wstring m_value;
};

//------------------------------------------------------------------

class DVAPI TPointerProperty : public TProperty
{
public:
	TPointerProperty(std::string name, void *value)
		: TProperty(name), m_value(value)
	{
	}

	TProperty *clone() const { return new TPointerProperty(*this); }

	void setValue(void *v) { m_value = v; }
	void *getValue() const { return m_value; }

	std::string getValueAsString()
	{
		return toString((unsigned long long)m_value);
	}

	void accept(Visitor &v) { v.visit(this); };

private:
	void *m_value;
};

//---------------------------------------------------------

class DVAPI TEnumProperty : public TProperty
{
public:
	typedef std::vector<std::wstring> Range;

	TEnumProperty(const std::string &name)
		: TProperty(name), m_index(-1) {}

	TEnumProperty(const std::string &name, const Range &range, const std::wstring &v)
		: TProperty(name), m_range(range), m_index(indexOf(v))
	{
		if (m_index < 0)
			throw RangeError();
	}

	TEnumProperty(const std::string &name,
				  Range::const_iterator i0, Range::const_iterator i1,
					const std::wstring &v)
		: TProperty(name), m_range(i0, i1), m_index(indexOf(v))
	{
		if (m_index < 0)
			throw RangeError();
	}

	TProperty *clone() const { return new TEnumProperty(*this); }

	int indexOf(const std::wstring &value)
	{
		Range::const_iterator it = std::find(m_range.begin(), m_range.end(), value);
		return (it == m_range.end()) ? -1 : it - m_range.begin();
	}

	bool isValue(const std::wstring &value)
	{
		bool ret = std::find(m_range.begin(), m_range.end(), value) != m_range.end();
		return ret;
	}

	void addValue(std::wstring value)
	{
		if (m_index == -1)
			m_index = 0;
		m_range.push_back(value);
	}

	void deleteAllValues()
	{
		m_range.clear();
		m_index = -1;
	}

	void setIndex(int index)
	{
		if (index < 0 || index >= (int)m_range.size())
			throw RangeError();
		m_index = index;
	}

	void setValue(const std::wstring &value)
	{
		int idx = indexOf(value);
		if (idx < 0)
			throw RangeError();
		m_index = idx;
	}

	const Range &getRange() const { return m_range; }
	std::wstring getValue() const { return (m_index < 0) ? L"" : m_range[m_index]; }
	std::string getValueAsString() { return toString(m_range[m_index]); }
	int getIndex() const { return m_index; }

	void accept(Visitor &v) { v.visit(this); }

	static void enableRangeSaving(bool on);
	static bool isRangeSavingEnabled();

private:
	Range m_range;
	int m_index;
};

//---------------------------------------------------------

class DVAPI TPropertyGroup
{
public:
	typedef std::vector<std::pair<TProperty *, bool>> PropertyVector;
	typedef std::map<std::string, TProperty *> PropertyTable;

	// exception
	class PropertyNotFoundError
	{
	};

	TPropertyGroup();
	virtual ~TPropertyGroup();

	virtual TPropertyGroup *clone() const;

	//! get ownership
	void add(TProperty *p);

	//! don't get ownership
	void bind(TProperty &p);

	//! returns 0 if the property doesn't exist
	TProperty *getProperty(std::string name);
	TProperty *getProperty(int i) { return (i >= (int)m_properties.size()) ? 0 : m_properties[i].first; }

	void setProperties(TPropertyGroup *g);

	void accept(TProperty::Visitor &v);

	int getPropertyCount() const { return (int)m_properties.size(); }

	void loadData(TIStream &is);
	void saveData(TOStream &os) const;

	void clear();

private:
	PropertyTable m_table;
	PropertyVector m_properties;

private:
	// not implemented
	TPropertyGroup(const TPropertyGroup &);
	TPropertyGroup &operator=(const TPropertyGroup &);
};

//---------------------------------------------------------

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif
