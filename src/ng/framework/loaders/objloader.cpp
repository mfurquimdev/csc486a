#include "ng/framework/loaders/objloader.hpp"

#include "ng/engine/filesystem/readfile.hpp"

#include "ng/engine/util/scopeguard.hpp"
#include "ng/engine/util/debug.hpp"

#include <stdexcept>
#include <sstream>

namespace ng
{

bool TryLoadObj(
        ObjShape& shape,
        IReadFile& objFile,
        std::string& error)
{
    error.clear();

    ObjShape newShape;

    int lineno = 1;

    auto linenoErrorScope = make_scope_guard([&]{
        if (error.length() > 0)
        {
            error = "Line " + std::to_string(lineno)
                  + ": " + error;
        }
    });

    std::vector<std::size_t> negativePositionsToPatch;
    std::vector<std::size_t> negativeTexcoordsToPatch;
    std::vector<std::size_t> negativeNormalsToPatch;

    int posIndexState = -1;
    int texIndexState = -1;
    int normIndexState = -1;

    for (std::string line; getline(line, objFile); lineno++)
    {
        // strip comment
        line = line.substr(0, line.find('#'));

        // strip leading whitespace
        std::size_t firstNonWs = line.find_first_not_of(" \t\n\r");
        line = line.substr(firstNonWs == std::string::npos ? 0 : firstNonWs);

        // strip trailing whitespace
        std::size_t lastNonWs = line.find_last_not_of(" \t\n\r");
        line = line.substr(0, lastNonWs == std::string::npos ? 0 : lastNonWs + 1);

        // ignore empty lines
        if (line.length() == 0)
        {
            continue;
        }

        // dump it into a stream
        std::stringstream ss;
        ss << line;

        // get the command
        std::string command;
        if (!(ss >> command))
        {
            error = "Couldn't read command";
            return false;
        }

        if (command == "v")
        {
            float f;
            int n;
            newShape.Positions.emplace_back();
            for (n = 0; ss >> f; n++)
            {
                if (n < 3)
                    newShape.Positions.back()[n] = f;
            }

            if (n != 3)
            {
                error = "Positions must be 3D";
                return false;
            }
        }
        else if (command == "vt")
        {
            float f;
            int n;
            newShape.Texcoords.emplace_back();
            for (n = 0; ss >> f; n++)
            {
                if (n < 2)
                    newShape.Texcoords.back()[n] = f;
            }

            if (n != 2)
            {
                error = "Texcoords must be 2D";
                return false;
            }
        }
        else if (command == "vn")
        {
            float f;
            int n;
            newShape.Normals.emplace_back();
            for (n = 0; ss >> f; n++)
            {
                if (n < 3)
                    newShape.Normals.back()[n] = f;
            }

            if (n != 3)
            {
                error = "Normals must be 3D";
                return false;
            }
        }
        else if (command == "f")
        {
            int pidx, tidx, nidx;
            bool haspidx = false, hastidx = false, hasnidx = false;

            int numVertices = 0;

            while (ss.peek() != std::stringstream::traits_type::eof())
            {
                if (!(ss >> pidx))
                {
                    error = "Couldn't read position index";
                    return false;
                }

                haspidx = true;

                if (ss.peek() == '/')
                {
                    ss.get();

                    if (ss.peek() != '/')
                    {
                        if (!(ss >> tidx))
                        {
                            error = "Couldn't read texcoord index";
                            return false;
                        }

                        hastidx = true;
                    }

                    if (ss.peek() == '/')
                    {
                        ss.get();
                        if (!(ss >> nidx))
                        {
                            error = "Couldn't read normal index";
                            return false;
                        }

                        hasnidx = true;
                    }
                }

                if (posIndexState == -1)
                {
                    posIndexState = haspidx;
                    newShape.HasPositionIndices = haspidx;
                }
                else if (posIndexState != haspidx)
                {
                    error = "Inconsistency in position index presence";
                    return false;
                }

                if (texIndexState == -1)
                {
                    texIndexState = hastidx;
                    newShape.HasTexcoordIndices = hastidx;
                }
                else if (texIndexState != hastidx)
                {
                    error = "Inconsistency in texcoord index presence";
                    return false;
                }

                if (normIndexState == -1)
                {
                    normIndexState = hasnidx;
                    newShape.HasNormalIndices = hasnidx;
                }
                else if (normIndexState != hasnidx)
                {
                    error = "Inconsistency in normal index presence";
                    return false;
                }

                if (haspidx)
                {
                    if (pidx < 0)
                    {
                        negativePositionsToPatch.push_back(newShape.Indices.size());
                    }
                    newShape.Indices.push_back(pidx < 0 ? pidx : pidx - 1);
                }

                if (hastidx)
                {
                    if (tidx < 0)
                    {
                        negativeTexcoordsToPatch.push_back(newShape.Indices.size());
                    }
                    newShape.Indices.push_back(tidx < 0 ? tidx : tidx - 1);
                }

                if (hasnidx)
                {
                    if (nidx < 0)
                    {
                        negativeNormalsToPatch.push_back(newShape.Indices.size());
                    }
                    newShape.Indices.push_back(nidx < 0 ? nidx : nidx - 1);
                }

                numVertices++;
            }

            if (newShape.VerticesPerFace == 0)
            {
                newShape.VerticesPerFace = numVertices;
            }
            else if (newShape.VerticesPerFace != numVertices)
            {
                error = "Inconsistent number of vertices per face";
                return false;
            }
        }
        else if (command == "g")
        {
            if (!(ss >> newShape.Name))
            {
                error = "Couldn't get group name";
                return false;
            }
        }
        else
        {
            error = "Unhandled command: ";
            error += command;
            return false;
        }

        if (ss.peek() != std::stringstream::traits_type::eof())
        {
            error = "Extra text on line";
            return false;
        }
    }

    // patch negative indices
    for (std::size_t i : negativePositionsToPatch)
    {
        newShape.Indices[i] = newShape.Positions.size() + newShape.Indices[i];
    }
    for (std::size_t i : negativeTexcoordsToPatch)
    {
        newShape.Indices[i] = newShape.Texcoords.size() + newShape.Indices[i];
    }
    for (std::size_t i : negativeNormalsToPatch)
    {
        newShape.Indices[i] = newShape.Normals.size() + newShape.Indices[i];
    }

    shape = std::move(newShape);
    return true;
}

void LoadObj(
        ObjShape& shape,
        IReadFile& objFile)
{
    std::string error;
    if (!TryLoadObj(shape, objFile, error))
    {
        throw std::runtime_error(error);
    }
}

} // end namespace ng
