#include "hierarchy.h"

#include "application.h"


Hierarchy::Hierarchy(MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_mesh_application(mesh_application),
  m_has_populated_hierarchy_model(false),
  m_hierarchy_popup_menu(NULL)
{
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 9, 0, 0);
	setLayout(m_layout);

	m_hierarchy_model = new Wt::WStandardItemModel(m_layout);
	m_hierarchy_model->setSortRole(HIERARCHY_ITEM_TREE_NUMBER_ROLE);
	m_hierarchy_tree_view = new Wt::WTreeView();
	m_hierarchy_tree_view->setModel(m_hierarchy_model);
	m_hierarchy_tree_view->setSelectionMode(Wt::SingleSelection);

	m_layout->addWidget(m_hierarchy_tree_view);
	m_hierarchy_tree_view->expanded().connect(this, &Hierarchy::TreeItemExpanded);
	m_hierarchy_tree_view->clicked().connect(this, &Hierarchy::TreeItemClicked);
}

Hierarchy::~Hierarchy()
{
	delete m_hierarchy_popup_menu;
}

void Hierarchy::PopulateHierarchy()
{
    if (m_has_populated_hierarchy_model)
    {
        return;
    }

    Wt::WString query = Wt::WString::tr("HierarchyTopNodesQuery");

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }
    
    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value id_value = source_object.getValue("id");
        std::string id_value_string = id_value.getString();
        if (id_value_string.empty())
            continue;
        
        std::string tree_number_value_string;
        if (source_object.member("tree_numbers"))
        {
            const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
            const Json::Array tree_numbers_array = tree_numbers_value.getArray();
            Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
            for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
            {
                const Json::Value tree_number_value = *tree_numbers_iterator;
                tree_number_value_string = tree_number_value.getString();
                if (std::string::npos != tree_number_value_string.find('.'))
                    continue; //skip child tree_numbers for meshes that also have top-level tree_numbers
                    
                //We have a top-level tree_number!
                const Json::Value name_value = source_object.getValue("name");
                
                std::stringstream node_text;
                node_text << name_value.getString();
                if (!tree_number_value_string.empty())
                {
                    node_text << " [" << tree_number_value_string << "]";
                }

                Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
                AddChildPlaceholderIfNeeded(source_object, tree_number_value_string, item);
                item->setData(boost::any(tree_number_value_string), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
                item->setData(boost::any(id_value_string), HIERARCHY_ITEM_ID_ROLE);
                m_hierarchy_model->setItem(row++, 0, item);
            }
        }
    }
    m_hierarchy_model->sort(0);
    m_has_populated_hierarchy_model = true;
}

void Hierarchy::ClearMarkedItems()
{
	std::vector<Wt::WStandardItem*>::iterator it = m_marked_hierarchy_items.begin();
	for (; it!=m_marked_hierarchy_items.end(); ++it)
	{
		(*it)->setStyleClass("");
	}
	m_marked_hierarchy_items.clear();
}

void Hierarchy::Collapse()
{
    PopulateHierarchy(); //Just in case it isn't populated yet

    int row = 0;
    Wt::WModelIndex index;
    while (true)
    {
        index = m_hierarchy_model->index(row, 0);
        if (!index.isValid())
            break;
        m_hierarchy_tree_view->collapse(index);
        row++;
    }
}

void Hierarchy::ExpandToTreeNumber(const std::string& tree_number_string)
{
    Wt::WModelIndex model_index;
    ExpandTreeNumberRecursive(tree_number_string, model_index);

	Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(model_index);
    if (standard_item)
    {
		m_marked_hierarchy_items.push_back(standard_item);
        standard_item->setStyleClass("marked_item");
    }
}

void Hierarchy::TreeItemExpanded(const Wt::WModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item || !standard_item->hasChildren())
    {
        return;
    }

    Wt::WStandardItem* possible_placeholder = standard_item->child(0, 0);
    if (!possible_placeholder || //We don't have a children placeholder. This item should not be populated by children 
        !possible_placeholder->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE).empty()) //This is a real child, not a placeholder. No need to populate children one more time.
    {
        return;
    }

    //Remove placeholder
    possible_placeholder = standard_item->takeChild(0, 0);
    delete possible_placeholder;

    std::string parent_tree_number_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE));
    //Fetch all children from ElasticSearch
    Wt::WString query = Wt::WString::tr("HierarchyChildrenQuery").arg(parent_tree_number_string);

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }

    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();

    bool added_items = false;
    int row = 0;
    Json::Array::const_iterator iterator = hits_array.begin();
    for (; iterator!=hits_array.end(); ++iterator)
    {
        const Json::Value hit_value = *iterator;
        const Json::Object hit_value_object = hit_value.getObject();
        const Json::Value source_value = hit_value_object.getValue("_source");
        const Json::Object source_object = source_value.getObject();

        const Json::Value id_value = source_object.getValue("id");
        std::string id_value_string = id_value.getString();
        if (id_value_string.empty())
            continue;
        
        std::string tree_number_value_string;
        std::string possible_parent_tree_number_string;
        const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
        const Json::Array tree_numbers_array = tree_numbers_value.getArray();
        Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
        for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
        {
            const Json::Value tree_number_value = *tree_numbers_iterator;
            tree_number_value_string = tree_number_value.getString();
            GetParentTreeNumber(tree_number_value_string, possible_parent_tree_number_string);
            if (EQUAL == parent_tree_number_string.compare(possible_parent_tree_number_string)) //This three_number matches the parent, add it as a child
            {
                const Json::Value name_value = source_object.getValue("name");
                
                std::stringstream node_text;
                node_text << name_value.getString();
                if (!tree_number_value_string.empty())
                {
                    node_text << " [" << tree_number_value_string << "]";
                }

                Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
                AddChildPlaceholderIfNeeded(source_object, tree_number_value_string, item);
                item->setData(boost::any(tree_number_value_string), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
                item->setData(boost::any(id_value_string), HIERARCHY_ITEM_ID_ROLE);
                standard_item->setChild(row++, 0, item);
                added_items = true;
            }
        }
    }
    
    if (added_items)
    {
        m_hierarchy_model->sort(0);
    }
}

void Hierarchy::TreeItemClicked(const Wt::WModelIndex& index, const Wt::WMouseEvent& mouse)
{
    if (!index.isValid())
    {
        return;
    }
    
    Wt::WStandardItem* standard_item = m_hierarchy_model->itemFromIndex(index);
    if (!standard_item)
    {
        return;
    }
    
    if (m_hierarchy_popup_menu)
    {
        delete m_hierarchy_popup_menu;
    }

    m_hierarchy_popup_menu = new Wt::WPopupMenu;
    m_hierarchy_popup_menu->setAutoHide(true, 1000);

    m_popup_menu_id_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_ID_ROLE));

    Wt::WString soek = Wt::WString::tr("SearchFromHierarchy").arg(standard_item->text().toUTF8());
    m_hierarchy_popup_menu->addItem(soek)->triggered().connect(this, &Hierarchy::PopupMenuTriggered);

    m_hierarchy_popup_menu->popup(mouse);
}

void Hierarchy::PopupMenuTriggered(Wt::WMenuItem* item)
{
    if (item && !m_popup_menu_id_string.empty())
    {
        m_mesh_application->ClearLayout();
		m_mesh_application->SetActiveTab(MeSHApplication::TAB_INDEX_SEARCH);
		m_mesh_application->SearchMesh(m_popup_menu_id_string);
    }
}

void Hierarchy::ExpandTreeNumberRecursive(const std::string& current_tree_number_string, Wt::WModelIndex& model_index)
{
    std::string parent_tree_number_string;
    GetParentTreeNumber(current_tree_number_string, parent_tree_number_string);
    bool top_level = parent_tree_number_string.empty();
    if (!top_level) //We're not at top-level yet
    {
        ExpandTreeNumberRecursive(parent_tree_number_string, model_index); //Recurse, depth-first
    }

    if (FindChildModelIndex(current_tree_number_string, top_level, model_index))
    {
        TreeItemExpanded(model_index);
        m_hierarchy_tree_view->expand(model_index);
    }
}

bool Hierarchy::FindChildModelIndex(const std::string& tree_number_string, bool top_level, Wt::WModelIndex& index)
{
    int row = 0;
    Wt::WModelIndex child_index;
    Wt::WStandardItem* standard_item;
    std::string item_tree_number_string;
    while (true)
    {
        child_index = top_level ? m_hierarchy_model->index(row, 0) : index.child(row, 0);
        if (!child_index.isValid())
        {
            return false;
        }

        standard_item = m_hierarchy_model->itemFromIndex(child_index);
        if (!standard_item)
        {
            return false;
        }

        item_tree_number_string = boost::any_cast<std::string>(standard_item->data(HIERARCHY_ITEM_TREE_NUMBER_ROLE));
        if (EQUAL == tree_number_string.compare(item_tree_number_string))
        {
            index = child_index;
            return true;
        }

        row++;
    }
}

bool Hierarchy::AddChildPlaceholderIfNeeded(const Json::Object& source_object, const std::string& current_tree_number_string, Wt::WStandardItem* current_item)
{
    bool added_placeholder = false;
    //Check if we have a matching child in the child_tree_numbers array
    if (source_object.member("child_tree_numbers"))
    {
        std::string possible_parent_tree_number_string;

        const Json::Value child_tree_numbers_value = source_object.getValue("child_tree_numbers");
        const Json::Array child_tree_numbers_array = child_tree_numbers_value.getArray();
        Json::Array::const_iterator child_iterator = child_tree_numbers_array.begin();
        for (; child_iterator!=child_tree_numbers_array.end(); ++child_iterator)
        {
            const Json::Value child_tree_number_value = *child_iterator;
            GetParentTreeNumber(child_tree_number_value.getString(), possible_parent_tree_number_string);
            if (EQUAL == current_tree_number_string.compare(possible_parent_tree_number_string))
            {
                Wt::WStandardItem* child_item = new Wt::WStandardItem(Wt::WString("")); //Placeholder, adds the [+]-icon
                current_item->setChild(0, 0, child_item);
                added_placeholder = true;
            }
        }
    }
    return added_placeholder;
}

void Hierarchy::GetParentTreeNumber(const std::string& child_tree_number, std::string& parent_tree_number)
{
    size_t substring_length = child_tree_number.find_last_of('.');
    if (std::string::npos==substring_length && 1<child_tree_number.length()) //If length==1, we have a forced topnode. We know they do not have a parent
    {
        substring_length = 1; //If we are forcing topnodes, "D" is the valid parent for "D01" (without dot..)
    }
    parent_tree_number = (std::string::npos==substring_length) ? "" : child_tree_number.substr(0, substring_length);
}
