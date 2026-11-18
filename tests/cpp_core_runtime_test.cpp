#include <iostream>
#include <cmath>
#include "../packs/cpp/code/core_runtime.h"
#define CHECK(expr) do { if(!(expr)) { std::cerr<<"Line "<<__LINE__<<": "<<#expr<<'\n'; return 1; } } while(0)
int main() {
    HiasmStateStore first, second;
    CHECK(!hiasm_current_state);
    {
        struct Sample { const char* operation; double left, right, expected; };
        const Sample samples[] = {
            {"+", 7, 2, 9}, {"-", 7, 2, 5}, {"*", 7, 2, 14}, {"/", 7, 2, 3.5},
            {"and", 6, 3, 2}, {"or", 6, 3, 7}, {"xor", 6, 3, 5},
            {"div", -7, 2, -3}, {"mod", -7, 2, -1}, {"shl", 3, 2, 12}, {"shr", 12, 2, 3},
            {"x^y", -2, 3, -8}, {"cos", 0, 0, 1}, {"sin", 0, 0, 0}, {"tg", 0, 0, 0},
            {"ctg", std::acos(-1.0)/4, 0, 1}, {"acos", 1, 0, 0}, {"asin", 0, 0, 0},
            {"atan", 1, 1, std::acos(-1.0)/4}, {"ch", 0, 0, 1}, {"sh", 0, 0, 0}, {"th", 0, 0, 0},
            {"cth", 1, 0, 1/std::tanh(1)}, {"ach", 1, 0, 0}, {"ash", 0, 0, 0}, {"ath", 0, 0, 0},
            {"acth", 2, 0, std::atanh(0.5)}, {"log", 2, 8, 3}, {"lg", 100, 0, 2}, {"ln", 1, 0, 0},
            {"exp", 0, 0, 1}, {"sqr", -3, 0, 9}, {"sqrt", 9, 0, 3}, {"abs", -3, 0, 3},
            {"sign", -3, 0, -1}, {"roun", 5, 2, 4}, {"frac", -5, 2, -1}, {"trun", -5, 2, -4},
            {"min", 7, 2, 2}, {"max", 7, 2, 7}, {"odd", -3, 0, 1}, {"even", -4, 0, 1},
            {"floor", -2.5, 0, -3}, {"ceil", -2.5, 0, -2}
        };
        HiasmMath math{7,7,false};
        for(const auto& sample : samples) {
            CHECK(math.calculate(sample.operation,sample.left,sample.right));
            if(std::abs(math.value-sample.expected)>1e-10) {
                std::cerr<<sample.operation<<": "<<math.value<<" != "<<sample.expected<<'\n'; return 1;
            }
        }
        math.degrees=true;
        CHECK(math.calculate("sin",90,0) && std::abs(math.value-1)<1e-12);
        CHECK(math.calculate("acos",0,0) && std::abs(math.value-90)<1e-12);
        CHECK(math.calculate("atan",1,1) && std::abs(math.value-45)<1e-12);
        CHECK(math.calculate("div",7.5,2.5) && math.value==4);
        CHECK(math.calculate("shr",-1,1) && math.value>0);
        CHECK(math.calculate("shl",1,-1) && math.value==static_cast<double>(INT64_MIN));
        CHECK(math.calculate("+",2.5,0) && math.output(true)==hiasm_value(2));
        CHECK(math.calculate("+",3.5,0) && math.output(true)==hiasm_value(4));
        CHECK(math.calculate("+",2147483648.0,0) && math.output(true)==hiasm_value(INT32_MIN));
        const Sample errors[] = {
            {"/",1,0,0}, {"div",1,0,0}, {"mod",1,0,0}, {"div",-9223372036854775808.0,-1,0},
            {"and",1e100,1,0}, {"x^y",-2,0.5,0}, {"x^y",0,-1,0}, {"ctg",0,0,0},
            {"acos",2,0,0}, {"asin",-2,0,0}, {"cth",0,0,0}, {"ach",0,0,0},
            {"ath",1,0,0}, {"acth",-1,0,0}, {"log",1,2,0}, {"log",2,0,0},
            {"lg",0,0,0}, {"ln",-1,0,0}, {"exp",1e100,0,0}, {"sqrt",-1,0,0},
            {"roun",1,0,0}, {"frac",1,0,0}, {"trun",1,0,0}, {"unknown",1,1,0}
        };
        for(const auto& sample : errors) {
            CHECK(!math.calculate(sample.operation,sample.left,sample.right));
            CHECK(math.error && math.value==7 && std::holds_alternative<std::monostate>(math.output(false)));
            math.clear(); CHECK(!math.error && math.output(false)==hiasm_value(7.0));
        }
        CHECK(!math.calculate("+",std::numeric_limits<double>::infinity(),0));
        CHECK(!math.calculate("/",1,std::numeric_limits<double>::quiet_NaN()));
        math.setDefault(42); CHECK(math.value==42 && !math.error);
    }
    {
        HiasmStateScope scope(first);
        auto& memory=hiasm_state("memory",hiasm_value(7));
        CHECK(std::holds_alternative<std::int64_t>(memory));
        memory=hiasm_value(u8"Привет 世界");
        const auto snapshot=memory;
        memory=hiasm_value(3.5);
        CHECK(hiasm_text_value(snapshot)==u8"Привет 世界" && hiasm_number_value(memory)==3.5);
        CHECK(hiasm_number_value(" 42.5 ")==42.5 && hiasm_number_value("42a")==0 && hiasm_number_value("")==0);
        CHECK(hiasm_counter_input(std::numeric_limits<double>::quiet_NaN())==0);
        CHECK(hiasm_counter_input(std::numeric_limits<double>::infinity())==0);
        CHECK(hiasm_counter_input(1e50)==0 && hiasm_counter_input(-3.5)==-4);
        CHECK(hiasm_counter_input(2.5)==2 && hiasm_counter_input(3.5)==4 && hiasm_counter_input(-2.5)==-2);
        HiasmCounter counter{0,0,3,1,false};
        CHECK(counter.next(false)==1 && counter.next(false)==2 && counter.next(false)==3 && counter.next(false)==0);
        CHECK(counter.next(true)==3);
        counter.reset(); CHECK(counter.value==0);
        counter.descending=true; counter.reset(); CHECK(counter.value==3);
        CHECK(counter.next(false)==2 && counter.next(true)==3);
        counter.step=5; CHECK(counter.next(false)==3);
        counter={INT32_MAX,INT32_MIN,INT32_MAX,1,false};
        CHECK(counter.next(false)==INT32_MIN && counter.next(true)==INT32_MAX);
        {
            HiasmStateScope nested(second);
            CHECK(hiasm_state("memory",hiasm_value(7))==hiasm_value(7));
        }
        CHECK(hiasm_state("memory",hiasm_value(0))==hiasm_value(3.5));
        auto& loop=hiasm_state("loop",HiasmLoopState{});
        {
            HiasmLoopScope outer(loop,3,0,-1);
            CHECK(outer.run->valid());
            outer.run->advance(); CHECK(outer.run->index==2);
            {
                HiasmLoopScope inner(loop,0,3,1);
                loop.stop(); CHECK(!inner.run->valid() && outer.run->valid());
            }
            loop.stop(); CHECK(!outer.run->valid());
        }
        CHECK(loop.active.empty());
        { HiasmLoopScope zero(loop,0,3,0); CHECK(!zero.run->valid()); }
        { HiasmLoopScope over(loop,INT64_MAX-1,INT64_MAX,2); over.run->advance(); CHECK(!over.run->valid()); }
        { HiasmLoopScope under(loop,INT64_MIN+1,INT64_MIN,-2); under.run->advance(); CHECK(!under.run->valid()); }
    }
    CHECK(!hiasm_current_state);
    std::cout<<"OK core state: typed memory, snapshots, isolated invocations, counter, loop stop/reentrancy/overflow\n";
}
