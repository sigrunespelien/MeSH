#include "elasticsearchutil.h"


ElasticSearchUtil::ElasticSearchUtil()
{
	m_es = std::make_unique<ElasticSearch>("elasticsearch:9200");
}

long ElasticSearchUtil::search(const std::string& index, const std::string& query, Json::Object& search_result)
{
	try
	{
        errno = 0;
		return m_es->search(index, query, search_result);
	}
    catch(...)
	{
		return 0L;
    }
}

bool ElasticSearchUtil::getDocument(const char* index, const char* id, Json::Object& msg)
{
	try
	{
        errno = 0;
		return m_es->getDocument(index, id, msg);
	}
    catch(...)
	{
		return false;
    }
}

bool ElasticSearchUtil::upsert(const std::string& index, const std::string& id, const Json::Object& jData)
{
	try
	{
        errno = 0;
		return m_es->upsert(index, id, jData);
	}
    catch(...)
	{
		return false;
    }
}
