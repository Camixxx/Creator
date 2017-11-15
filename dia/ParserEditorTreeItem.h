#ifndef TREEITEM_H
#define TREEITEM_H

#include <QList>
#include <QVariant>

//! [0]



class ParserEditorTreeItem
{
public:
	enum Type
	{
		VOID,
		NUMBER,
		STRING,
		DICTIONARY,
		ARRAY,
		EXPRESSION
	};

    explicit ParserEditorTreeItem(const QString &key, Type type, const QString &value);
	explicit ParserEditorTreeItem();
    ~ParserEditorTreeItem();

    void appendChild(ParserEditorTreeItem *child);
	void removeChildAt(int row);
	void removeChildrenAt(int row, int count);
	void replaceChildAt(int row, ParserEditorTreeItem *child);
	void insertChildBefore(int pos, ParserEditorTreeItem *child);

    ParserEditorTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
	void setData(int column, const QVariant &data);
	const QString &value() const { return _value; }
	void setValue(const QString &value) { _value = value; }
    int row() const;
    ParserEditorTreeItem *parentItem();
	int level();
	void clear();

	ParserEditorTreeItem *duplicate() const;

	QString typeString() const;
	const QString &key() const { return _key; }
	void setKey(const QString &key) { _key = key; }
	Type type() const { return _type; }
	void setType(Type type) { _type = type; }
	bool setTypeString(const QString &str);
	void rebuildArrayName();
	static QStringList allTypeStrings();

private:
    QList<ParserEditorTreeItem*> m_childItems;
	QString _key;
	Type _type;
	QString _value;
    ParserEditorTreeItem *m_parentItem;
};
//! [0]

#endif // TREEITEM_H
