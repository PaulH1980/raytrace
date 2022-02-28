#pragma once


namespace RayTrace
{


    class NonAssignable {
    protected:
        NonAssignable(const NonAssignable&) = default;
        //NonAssignable& operator=(const NonAssignable&) = default;
        NonAssignable& operator=(NonAssignable&&) = delete;
        NonAssignable( NonAssignable&&) = delete;
        NonAssignable() {}
    };

}