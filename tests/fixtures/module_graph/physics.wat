(module
    (import
        "math"
        "add"
        (func $add
            (param i32 i32)
            (result i32)
        )
    )

    (func
        (export "step")
        (param i32)
        (result i32)

        local.get 0
        i32.const 1
        call $add
    )
)
