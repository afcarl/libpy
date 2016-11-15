#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "gtest/gtest.h"
#include <Python.h>

#include "libpy/libpy.h"
#include "utils.h"

using py::operator""_p;

TEST(Layout, py_object) {
    EXPECT_TRUE(std::is_standard_layout<py::object>::value);
    EXPECT_EQ(sizeof(py::object), sizeof(PyObject*));
}

class Object : public testing::Test {
protected:
    py::object C;

    Object() : C(nullptr) {}

    virtual void SetUp() {
        PyObject *ns = PyEval_GetBuiltins();
        // create a type so that it has a mutable dict
        this->C = PyRun_String("type('C', (), {})",
                               Py_eval_input,
                               ns,
                               ns);
    }

    virtual void TearDown() {
        this->C.decref();
    }
};

TEST_F(Object, ostream_nonnull) {
    std::stringstream stream;
    std::unordered_map<std::string, py::object> subtests = {{"c", 'c'_p},
                                                            {"1", 1_p},
                                                            {"1.5", 1.5_p},
                                                            {"test", "test"_p}};

    for (const auto &kv : subtests) {
        for (const auto &as_nonnull : {false, true}) {
            py::object ob = std::get<1>(kv);
            if (!as_nonnull) {
                stream << ob;
            } else {
                stream << ob.as_nonnull();
            }
            EXPECT_EQ(stream.str(), std::get<0>(kv)) <<
                "as_nonnull = " << as_nonnull << '\n';
            stream.str("");  // clear the stream's contents
        }
    }
}

TEST_F(Object, ostream_nullptr) {
    std::stringstream stream;
    stream << py::object(nullptr);
    EXPECT_EQ(stream.str(), "<NULL>");
}

TEST_F(Object, hasattr) {
    py::object ob = "test"_p;
    py::object attrs = ob.dir().iter();
    py::tmpref<py::object> attr;

    while ((attr = attrs.next())) {
        EXPECT_TRUE(ob.hasattr(attr));
    }
    ASSERT_FALSE(PyErr_Occurred());

    for (const auto &attr : {"invalid1"_p, "invalid2"_p}) {
        EXPECT_FALSE(ob.hasattr(attr));
    }
}

TEST_F(Object, getattr) {
    py::object ob = "test"_p;
    py::object attrs = ob.dir().iter();
    py::tmpref<py::object> attr;

    while ((attr = attrs.next())) {
        EXPECT_IS_NOT(ob.getattr(attr), nullptr);
    }
    ASSERT_FALSE(PyErr_Occurred());

    for (const auto &attr : {"invalid1"_p, "invalid2"_p}) {
        EXPECT_IS(ob.getattr(attr), nullptr);
        PyErr_Clear();
    }
}

TEST_F(Object, setattr_delattr) {
    ASSERT_FALSE(this->C.hasattr("test"_p));
    ASSERT_EQ(this->C.setattr("test"_p, 1_p), 0);
    EXPECT_TRUE(this->C.hasattr("test"_p));
    EXPECT_IS(this->C.getattr("test"_p), 1_p);
    ASSERT_EQ(this->C.delattr("test"_p), 0);
    EXPECT_FALSE(this->C.hasattr("test"_p));
}
