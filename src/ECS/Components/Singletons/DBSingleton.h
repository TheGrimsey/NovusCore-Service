#pragma once
#include <Database/DBConnection.h>
#include <Database/DBTypes.h>

struct DBSingleton
{
public:
    DBSingleton() : auth(2), realm(2), world(2) { }

    DBConnection auth;
    DBConnection realm;
    DBConnection world;
};