/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2018                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

namespace openspace::luascriptfunctions {

/**
 * \ingroup LuaScripts
 * loadKernel(string):
 * Loads the provided SPICE kernel by name. The name can contain path tokens, which are
 * automatically resolved.
 */

int loadKernel(lua_State* L) {
    ghoul::lua::checkArgumentsAndThrow(L, 1, "lua::loadKernel");

    bool isString = (lua_isstring(L, -1) == 1);
    if (!isString) {
        LERROR(ghoul::lua::errorLocation(L) << "Expected argument of type 'string'");
        return 0;
    }

    std::string argument = ghoul::lua::checkStringAndPop(L);
    if (!FileSys.fileExists(argument)) {
        return luaL_error(L, "Kernel file '%s' did not exist", argument.c_str());
    }
    unsigned int result = SpiceManager::ref().loadKernel(argument);

    lua_pushnumber(L, result);

    ghoul_assert(lua_gettop(L) == 1, "Incorrect number of items left on stack");
    return 1;
}

/**
 * unloadKernel({string, number}):
 * Unloads the provided SPICE kernel. The name can contain path tokens, which are
 * automatically resolved.
 */
int unloadKernel(lua_State* L) {
    ghoul::lua::checkArgumentsAndThrow(L, 1, "lua::loadKernel");

    bool isString = (lua_isstring(L, -1) == 1);
    bool isNumber = (lua_isnumber(L, -1) == 1);

    if (!isString && !isNumber) {
        LERRORC(
            "loadKernel",
            ghoul::lua::errorLocation(L) <<
            "Expected argument of type 'string' or 'number'"
        );
        lua_settop(L, 0);
        ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
        return 0;
    }

    if (isString) {
        std::string argument = lua_tostring(L, -1);
        SpiceManager::ref().unloadKernel(argument);
    }

    if (isNumber) {
        unsigned int argument = static_cast<unsigned int>(lua_tonumber(L, -1));
        SpiceManager::ref().unloadKernel(argument);
    }

    lua_settop(L, 0);

    ghoul_assert(lua_gettop(L) == 0, "Incorrect number of items left on stack");
    return 0;
}

} // namespace openspace::luascriptfunctions
