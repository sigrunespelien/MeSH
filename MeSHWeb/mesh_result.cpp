#include "mesh_result.h"

#include <Wt/WHBoxLayout>
#include <Wt/WImage>
#include <Wt/WStringListModel>
#include <Wt/Utils>

#include "application.h"
#include "log.h"


MeshResult::MeshResult(MeSHApplication* mesh_application, Wt::WContainerWidget* parent)
: Wt::WContainerWidget(parent),
  m_mesh_application(mesh_application)
{
	setStyleClass("search-result");
	m_layout = new Wt::WVBoxLayout();
	m_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(m_layout);

	m_nor_term_panel = new Wt::WPanel();
	m_nor_term_panel->setCollapsible(true);
	Wt::WContainerWidget* nor_term_container = new Wt::WContainerWidget();
	m_nor_term_panel_layout = new Wt::WVBoxLayout();
	m_nor_term_panel_layout->setContentsMargins(0, 0, 0, 0);
	nor_term_container->setLayout(m_nor_term_panel_layout);
	m_nor_term_panel->setCentralWidget(nor_term_container);

	Wt::WImage* nor_flag = new Wt::WImage("images/nor.png");
	m_nor_term_panel->titleBarWidget()->insertWidget(0, nor_flag);
	nor_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
	nor_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));

	m_layout->addWidget(new Wt::WText(Wt::WString::tr("PreferredNorwegianTerm")));
	m_layout->addWidget(m_nor_term_panel);

	m_nor_description_label = new Wt::WText(Wt::WString::tr("NorwegianDescription"));
	m_nor_description_label->setStyleClass("scope scope-class");
	m_layout->addWidget(m_nor_description_label);
	m_nor_description_text = new Wt::WText();
	m_nor_description_text->setStyleClass("scope-note scope-class");
	m_layout->addWidget(m_nor_description_text);

	m_eng_term_panel = new Wt::WPanel();
	m_eng_term_panel->setCollapsible(true);
	Wt::WContainerWidget* eng_term_container = new Wt::WContainerWidget();
	m_eng_term_panel_layout = new Wt::WVBoxLayout();
	m_eng_term_panel_layout->setContentsMargins(0, 0, 0, 0);
	eng_term_container->setLayout(m_eng_term_panel_layout);
	m_eng_term_panel->setCentralWidget(eng_term_container);

	Wt::WImage* eng_flag = new Wt::WImage("images/eng.png");
	m_eng_term_panel->titleBarWidget()->insertWidget(0, eng_flag);
	eng_flag->setHeight(Wt::WLength(1.0, Wt::WLength::FontEm));
	eng_flag->setMargin(Wt::WLength(3.0, Wt::WLength::Pixel));

	m_layout->addWidget(new Wt::WText(Wt::WString::tr("PreferredEnglishTerm")));
	m_layout->addWidget(m_eng_term_panel);

	m_eng_description_label = new Wt::WText(Wt::WString::tr("EnglishDescription"));
	m_eng_description_label->setStyleClass("scope scope-class");
	m_layout->addWidget(m_eng_description_label);
	m_eng_description_text = new Wt::WText();
	m_eng_description_text->setStyleClass("scope-note scope-class");
	m_layout->addWidget(m_eng_description_text);

	m_see_related_text = new Wt::WText(Wt::WString::tr("SeeRelated"));
	m_layout->addWidget(m_see_related_text);
	m_see_related_container = new Wt::WContainerWidget();
	m_layout->addWidget(m_see_related_container);

	m_layout->addSpacing(Wt::WLength(1.0, Wt::WLength::FontEm));

#if 0
	m_layout->addWidget(new Wt::WText(Wt::WString::tr("Links")));
#endif
	m_links = new Links();
	m_layout->addWidget(m_links);

	m_layout->addSpacing(Wt::WLength(2.0, Wt::WLength::FontEm));
	
	m_hierarchy_model = new Wt::WStandardItemModel(m_layout);
	m_hierarchy_model->setSortRole(HIERARCHY_ITEM_TREE_NUMBER_ROLE);
	m_hierarchy_tree_view = new Wt::WTreeView();
	m_hierarchy_tree_view->setModel(m_hierarchy_model);
	m_hierarchy_tree_view->setSelectionMode(Wt::SingleSelection);
	m_layout->addWidget(m_hierarchy_tree_view);
}

MeshResult::~MeshResult()
{
}

void MeshResult::ClearLayout()
{
    m_nor_term_panel->setTitle("");
    m_nor_term_panel->expand();
    m_nor_term_panel_layout->clear();
    
    m_nor_description_text->setText("");
    m_nor_description_text->hide();

    m_eng_term_panel->setTitle("");
    m_eng_term_panel->collapse();
    m_eng_term_panel_layout->clear();

    m_eng_description_text->setText("");
    m_eng_description_text->hide();

	m_hierarchy_model->clear();

	m_see_related_text->hide();
	m_see_related_container->clear();
	
    m_links->clear();

    hide();
}

void MeshResult::OnSearch(const Wt::WString& mesh_id, const std::string& search_text)
{
	std::string preferred_term;
    Wt::WString preferred_eng_term;

    m_nor_term_panel_layout->clear();
    m_eng_term_panel_layout->clear();
	m_hierarchy_model->clear();
	m_see_related_text->hide();
	m_see_related_container->clear();

	m_nor_description_label->hide();
	m_nor_description_text->hide();
	m_eng_description_label->hide();
	m_eng_description_text->hide();

	Wt::WString query = Wt::WString::tr("SearchFilterQuery").arg(mesh_id.toUTF8());

    Json::Object search_result;
	ElasticSearchUtil* es_util = m_mesh_application->GetElasticSearchUtil();
    long result_size = es_util->search("mesh", LANGUAGE, query.toUTF8(), search_result);
    if (0 == result_size)
    {
        return;
    }
    
    Log log(m_mesh_application);
	log.LogSearch(mesh_id.toUTF8());

    Wt::WStringListModel* non_preferred_nor_terms = new Wt::WStringListModel();
    Wt::WStringListModel* non_preferred_eng_terms = new Wt::WStringListModel();
	bool found_norwegian_preferred_term = false;

    const Json::Value value = search_result.getValue("hits");
    const Json::Object value_object = value.getObject();
    const Json::Value hits_value = value_object.getValue("hits");
    const Json::Array hits_array = hits_value.getArray();
    const Json::Value hit_value = hits_array.first();
    const Json::Object hit_value_object = hit_value.getObject();
    const Json::Value source_value = hit_value_object.getValue("_source");
    const Json::Object source_object = source_value.getObject();
	if (!source_object.member("concepts")) //Probably a top-node
		return;

    const Json::Value concepts_value = source_object.getValue("concepts");
    const Json::Array concepts_array = concepts_value.getArray();

    Json::Array::const_iterator concept_iterator = concepts_array.begin();
    for (; concept_iterator!=concepts_array.end(); ++concept_iterator)
    {
        const Json::Value concept_value = *concept_iterator;
        const Json::Object concept_object = concept_value.getObject();
        const Json::Value preferred_concept_value = concept_object.getValue("preferred");
        bool is_preferred_concept = (EQUAL == preferred_concept_value.getString().compare("yes"));
        if (is_preferred_concept && concept_object.member("description"))
        {
            const Json::Value description_value = concept_object.getValue("description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            
			m_nor_description_label->show();
            m_nor_description_text->setTextFormat(Wt::PlainText);
            m_nor_description_text->setText(Wt::WString::fromUTF8(description_str));
            m_nor_description_text->show();
        }
        if (is_preferred_concept && concept_object.member("english_description"))
        {
            const Json::Value description_value = concept_object.getValue("english_description");
            std::string description_str = description_value.getString();
            boost::replace_all(description_str, "\\n", "\n");
            
			m_eng_description_label->show();
            m_eng_description_text->setTextFormat(Wt::PlainText);
            m_eng_description_text->setText(Wt::WString::fromUTF8(description_str));
            m_eng_description_text->show();
        }

        const Json::Value terms_value = concept_object.getValue("terms");
        const Json::Array terms_array = terms_value.getArray();
        Json::Array::const_iterator term_iterator = terms_array.begin();
        for (; term_iterator!=terms_array.end(); ++term_iterator)
        {
            const Json::Value term_value = *term_iterator;
            const Json::Object term_object = term_value.getObject();
            
            bool is_norwegian = false;
            if (term_object.member("language"))
            {
                const Json::Value language_value = term_object.getValue("language");
                is_norwegian = (EQUAL == language_value.getString().compare("nor"));
            }

            const Json::Value term_text_value = term_object.getValue("text");
            const Wt::WString term_text_str = Wt::WString::fromUTF8(term_text_value.getString());
            
            const Json::Value preferred_term_value = term_object.getValue("preferred");
            bool is_preferred_term = (EQUAL == preferred_term_value.getString().compare("yes"));
            if (is_preferred_concept && is_preferred_term)
            {
                Wt::WPanel* term_panel = (is_norwegian ? m_nor_term_panel : m_eng_term_panel);
                term_panel->setTitle(term_text_str);
				if (is_norwegian)
				{
					preferred_term = term_text_value.getString();
				}
                else if (!is_norwegian && preferred_eng_term.empty())
                {
					preferred_term = term_text_value.getString();
                    preferred_eng_term = term_text_str;
                }
                found_norwegian_preferred_term |= (is_norwegian!=false);
            }
            else
            {
                Wt::WStringListModel* term_list = (is_norwegian ? non_preferred_nor_terms :  non_preferred_eng_terms);
                term_list->addString(term_text_str);
            }
        }
    }

    m_nor_term_panel_layout->addWidget(new Wt::WText(Wt::WString::tr("NonPreferredNorwegianTerms")));
    m_eng_term_panel_layout->addWidget(new Wt::WText(Wt::WString::tr("NonPreferredEnglishTerms")));

    int i;
    for (i=0; i<2; i++)
    {
        const std::vector<Wt::WString>* non_preferred_term_list;
        Wt::WLayout* non_preferred_term_layout = NULL;

        switch(i)
        {
            case 0:
                non_preferred_term_list = &non_preferred_nor_terms->stringList();
                non_preferred_term_layout = m_nor_term_panel_layout;
                break;
                
            case 1:
                non_preferred_term_list = &non_preferred_eng_terms->stringList();
                non_preferred_term_layout = m_eng_term_panel_layout;
                break;
                
            default: break;
        }
        
        if (!non_preferred_term_layout)
            continue;
        
        std::vector<Wt::WString>::const_iterator non_preferred_term_iterator = non_preferred_term_list->begin();
        for ( ; non_preferred_term_iterator!=non_preferred_term_list->end(); ++non_preferred_term_iterator)
        {
            Wt::WString non_preferred_term_string = *non_preferred_term_iterator;
            Wt::WText* non_preferred_term_text = new Wt::WText(non_preferred_term_string, Wt::PlainText);
            non_preferred_term_layout->addWidget(non_preferred_term_text);
        }
    }

    if (found_norwegian_preferred_term)
	{
		m_nor_term_panel->expand();
		m_eng_term_panel->collapse();
	}
	else
	{
		m_nor_term_panel->setTitle(Wt::WString::tr("NotTranslated"));
		m_nor_term_panel->collapse();
		m_eng_term_panel->expand();
	}

	PopulateHierarchy(es_util, source_object);

	//See Related
	if (source_object.member("see_related"))
	{
		m_see_related_text->show();

		const Json::Value see_related_values = source_object.getValue("see_related");
		const Json::Array see_related_array = see_related_values.getArray();
		Json::Array::const_iterator see_related_iterator = see_related_array.begin();
		for (; see_related_iterator!=see_related_array.end(); ++see_related_iterator)
		{
			const Json::Value see_related_value = *see_related_iterator;

			std::string see_related_id = see_related_value.getString();
			std::string title;
	        Search::MeSHToName(es_util, see_related_id, title);
			std::string url = (Wt::WString::tr("MeshIdInternalPath")+"&"+Wt::WString::tr("MeshIdInternalPathParam").arg(see_related_id)).toUTF8();
			Wt::WAnchor* see_related_anchor = new Wt::WAnchor(Wt::WLink(Wt::WLink::InternalPath, url), Wt::WString::fromUTF8(title));
			see_related_anchor->setStyleClass("mesh-link");
			m_see_related_container->addWidget(see_related_anchor);
		}
	}

	//Links
    std::string url_encoded_term = Wt::Utils::urlEncode(preferred_eng_term.toUTF8());
    std::string url_encoded_filtertext = Wt::Utils::urlEncode(search_text);
	
	m_links->populate(mesh_id, preferred_term, url_encoded_term, url_encoded_filtertext);
    
    delete non_preferred_nor_terms;
    delete non_preferred_eng_terms;
    
	//Mark search-result in hierarchy search tab
	Hierarchy* hierarchy = m_mesh_application->GetHierarchy();
	hierarchy->ClearMarkedItems();
	hierarchy->Collapse();
	if (source_object.member("tree_numbers"))
	{
		const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
		const Json::Array tree_numbers_array = tree_numbers_value.getArray();
		Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
		for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
		{
			const Json::Value tree_number_value = *tree_numbers_iterator;
			hierarchy->ExpandToTreeNumber(tree_number_value.getString());
		}
	}
}

void MeshResult::RecursiveAddHierarchyItem(ElasticSearchUtil* es_util, int& row, std::map<std::string, Wt::WStandardItem*>& node_map, const std::string& tree_number, bool mark_item) {
	if (tree_number.empty() || //parent of top-node
		0<node_map.count(tree_number)) //Already added?
	{
		return;
	}

	//Make sure parent is added
	std::string parent_tree_number;
	Hierarchy::GetParentTreeNumber(tree_number, parent_tree_number);
	RecursiveAddHierarchyItem(es_util, row, node_map, parent_tree_number, false);

	std::string name;
	std::string mesh_id;
	Search::TreeNumberToName(es_util, tree_number, name, &mesh_id);
	std::stringstream node_text;
	node_text << name;
	if (!tree_number.empty())
	{
		node_text << " [" << tree_number << "]";
	}
    Wt::WStandardItem* item = new Wt::WStandardItem(Wt::WString::fromUTF8(node_text.str()));
	item->setData(boost::any(tree_number), HIERARCHY_ITEM_TREE_NUMBER_ROLE);
	item->setData(boost::any(mesh_id), HIERARCHY_ITEM_ID_ROLE);

	std::map<std::string,Wt::WStandardItem*>::iterator iter = node_map.find(parent_tree_number);
	if (node_map.end() == iter) //No parent -> top-node
	{
		m_hierarchy_model->setItem(row++, 0, item);
	}
	else
	{
		Wt::WStandardItem* parent_item = iter->second;
		parent_item->setChild(parent_item->rowCount(), 0, item);
		m_hierarchy_tree_view->expand(parent_item->index());
	}

	if (mark_item)
	{
		item->setStyleClass("marked_item");
	}

	node_map[tree_number] = item;
}

void MeshResult::PopulateHierarchy(ElasticSearchUtil* es_util, const Json::Object& source_object)
{
	m_hierarchy_model->clear();
	int row = 0;

	std::map<std::string,Wt::WStandardItem*> node_map;

	if (source_object.member("tree_numbers"))
	{
		const Json::Value tree_numbers_value = source_object.getValue("tree_numbers");
		const Json::Array tree_numbers_array = tree_numbers_value.getArray();
		Json::Array::const_iterator tree_numbers_iterator = tree_numbers_array.begin();
		for (; tree_numbers_iterator!=tree_numbers_array.end(); ++tree_numbers_iterator)
		{
			const Json::Value tree_number_value = *tree_numbers_iterator;
			RecursiveAddHierarchyItem(es_util, row, node_map, tree_number_value.getString(), true);
		}
	}
	if (source_object.member("child_tree_numbers"))
	{
		const Json::Value child_tree_numbers_value = source_object.getValue("child_tree_numbers");
		const Json::Array child_tree_numbers_array = child_tree_numbers_value.getArray();
		Json::Array::const_iterator child_tree_numbers_iterator = child_tree_numbers_array.begin();
		for (; child_tree_numbers_iterator!=child_tree_numbers_array.end(); ++child_tree_numbers_iterator)
		{
			const Json::Value child_tree_number_value = *child_tree_numbers_iterator;
			RecursiveAddHierarchyItem(es_util, row, node_map, child_tree_number_value.getString(), false);
		}
	}
}
