(module
    (import
        "A"
        "a"
        (func $a
            (result i32)
        )
    )

    (func
        (export "b")
        (result i32)

        call $a
    )
)
