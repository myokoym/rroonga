# Copyright (C) 2009-2012  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class TypeTest < Test::Unit::TestCase
  include GroongaTestUtils

  setup :setup_database

  def test_new
    type = Groonga::Type.new("user_id", :type => :integer)
    assert_equal("user_id", type.name)
  end

  def test_new_with_space_name
    exception = assert_raise(Groonga::InvalidArgument) do
      Groonga::Type.new("user id", :type => :integer)
    end
    message =
      "name can't start with '_' and contains only 0-9, A-Z, a-z, #, @, - or _"
    assert_match(/#{Regexp.escape(message)}/,
                 exception.message)
  end

  def test_size
    assert_equal(4, Groonga["Int32"].size)
  end

  def test_flags
    key_int = 0x01 << 3
    assert_equal(key_int, Groonga["Int32"].flags)
  end

  def test_fixed_size?
    assert_true(Groonga["Int32"].fixed_size?)
  end

  def test_not_fixed_size?
    assert_false(Groonga["ShortText"].fixed_size?)
  end

  def test_variable_size?
    assert_true(Groonga["ShortText"].variable_size?)
  end

  def test_not_variable_size?
    assert_false(Groonga["Int32"].variable_size?)
  end

  def test_unsigned_integer?
    assert_true(Groonga["UInt32"].unsigned_integer?)
  end

  def test_not_unsigned_integer?
    assert_false(Groonga["Int32"].unsigned_integer?)
  end

  def test_integer?
    assert_true(Groonga["Int32"].integer?)
  end

  def test_not_integer?
    assert_false(Groonga["UInt32"].integer?)
  end

  def test_float?
    assert_true(Groonga["Float"].float?)
  end

  def test_not_float?
    assert_false(Groonga["UInt32"].float?)
  end

  def test_geo_point?
    assert_true(Groonga["WGS84GeoPoint"].geo_point?)
  end

  def test_not_geo_point?
    assert_false(Groonga["UInt32"].geo_point?)
  end

  def test_builtins
    assert_equal_type("Object", Groonga::Type::OBJECT) # FIXME!!!
    assert_equal_type("Bool", Groonga::Type::BOOLEAN)
    assert_equal_type("Bool", Groonga::Type::BOOL)
    assert_equal_type("Int8", Groonga::Type::INT8)
    assert_equal_type("UInt8", Groonga::Type::UINT8)
    assert_equal_type("Int16", Groonga::Type::INT16)
    assert_equal_type("UInt16", Groonga::Type::UINT16)
    assert_equal_type("Int32", Groonga::Type::INT32)
    assert_equal_type("UInt32", Groonga::Type::UINT32)
    assert_equal_type("Int64", Groonga::Type::INT64)
    assert_equal_type("UInt64", Groonga::Type::UINT64)
    assert_equal_type("Float", Groonga::Type::FLOAT)
    assert_equal_type("Time", Groonga::Type::TIME)
    assert_equal_type("ShortText", Groonga::Type::SHORT_TEXT)
    assert_equal_type("Text", Groonga::Type::TEXT)
    assert_equal_type("LongText", Groonga::Type::LONG_TEXT)
  end

  def test_inspect
    assert_equal("#<Groonga::Type id: <#{Groonga::Type::LONG_TEXT}>, " +
                 "name: <LongText>, " +
                 "path: (temporary), " +
                 "domain: (nil), " +
                 "range: <2147483648>, " +
                 "flags: <KEY_VAR_SIZE>>",
                 context["<longtext>"].inspect)
  end

  data("builtin - Int32" => "Int32",
       "builtin - ShortText" => "ShortText",
       "builtin - Time" => "Time")
  def test_builtin?(name)
    type = Groonga[name]
    assert_predicate(type, :builtin?)
  end

  private
  def assert_equal_type(expected_name, id)
    type = Groonga[id]
    assert_equal(expected_name,
                 type ? type.name : type)
  end
end
