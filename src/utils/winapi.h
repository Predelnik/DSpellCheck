#pragma once
#include "enum_range.h"

// various winapi helpers
std::wstring get_edit_text(HWND edit);
bool move_file_and_reset_security_descriptor(const wchar_t* from, const wchar_t* to);

namespace WinApi
{
    class WinBase {
    public:
        virtual ~WinBase();
        void set_enabled(bool enabled);
        void init(HWND hwnd);

    private:
        virtual void check_hwnd() = 0;
        virtual void init_impl () {}

    protected:

        HWND m_hwnd = nullptr;
    };

    class ComboBox : public WinBase {
        void check_hwnd() override;

    public:
        template <typename EnumType>
        void add_items() {
            static_assert (std::is_enum_v<EnumType>);
            int i = 0;
            for (auto val : enum_range<EnumType>()) {
                ComboBox_AddString (m_hwnd, gui_string (val));
                ComboBox_SetItemData (m_hwnd, i, static_cast<std::underlying_type_t<EnumType>> (val));
                ++i;
            }
        }

        int current_index() const {
            return static_cast<int> (ComboBox_GetCurSel(m_hwnd));
        }

        int current_data() const {
            return static_cast<int> (ComboBox_GetItemData(m_hwnd, current_index ()));
        }

        void set_current_index(int index) {
            ComboBox_SetCurSel(m_hwnd, index);
        }

        std::optional<int> find_by_data(int data);
    };

    template <typename EnumType>
    class EnumComboBox : protected ComboBox {
        using Parent = ComboBox;
    public:
        using Parent::init;
        using Parent::set_enabled;

        EnumType current_data() const {
            return static_cast<EnumType>(Parent::current_data());
        }

        void set_index(EnumType value) {
            set_current_index(*find_by_data(static_cast<std::underlying_type_t<EnumType>>(value)));
        }

    private:
        void init_impl() override {
          this->add_items<EnumType>();
        }
    };
} // namespace WinApi
