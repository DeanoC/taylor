local image = require "image"
do
    local test = image.load("test_imageload.png")
    if test == nil then
        print("test_imageload.png unable to be loaded")
    end
    local w = test:width();
    local h = test:height();
    local d = test:depth();
    local s = test:slices();
    local format = test:format();
    local flags = test:flags()

    if w ~= 884 then
        print("test_imageload.png is " .. w .. " wide it should be 884")
    end
    if h ~= 406 then
        print("test_imageload.png is " .. h .. " high it should be 406")
    end
    if d ~= 1 then
        print("test_imageload.png is " .. d .. " depth it should be 1")
    end
    if s ~= 1 then
        print("test_imageload.png is " .. s .. " slices it should be 1")
    end
    if format ~= "R8G8B8A8_UNORM" then
        print("test_imageload.png is " .. format .. " and should be R8G8B8A8_UNORM")
    end
    if flags.Cubemap == true then
        print("test_imageload.png marked as a Cubemaps and shouldn't be")
    end
    if flags.HeaderOnly == true then
        print("test_imageload.png marked as a header only  and shouldn't be")
    end

    --image.saveAsDDS("test_savedds.dds", test)
    test:saveAsTGA("test_save.tga")
    test:saveAsBMP("test_save.bmp")
    test:saveAsPNG("test_save.png")
    test:saveAsJPG("test_save.jpg")
    test:saveAsKTX("test_save.ktx")
end

do
    local test = image.create2D(16, 16, "R8G8B8A8_UNORM")
    if test == nil then
        print("create2D fail")
    end
    for y = 1, 16 do
        for x = 1, 16 do
            local i = (y-1) * 16 + (x-1)
            test:setPixelAt(i, (x-1.0)/15.0, (y-1.0)/15.0, (x-1.0)/15.0, 1.0)
        end
    end
    test:saveAsKTX("test_save_col_16x16.ktx")
end

do
    local test = image.load("test_pvt_col_mm_16x16.ktx")
    if test == nil then
        print("load test_pvt_col_mm_16x16.ktx fail")
    end
    test:saveAsKTX("test_save_col_mm_16x16.ktx")
end

do
    local test = image.load("test_pvtr8g8b8a8mm.ktx")
    if test == nil then
        print("test_pvtr8g8b8a8mm.ktx unable to be loaded")
    end

    test:saveAsPNG("test_save_pvtr8g8b8a8mm.png")
    test:saveAsKTX("test_save_pvtr8g8b8a8mm.ktx")
end

do
    local test = image.create(884, 406, 1, 1, "R8G8B8A8_UNORM")
    if test == nil then
        print("unable to be create image")
    end
    local w = test:width();
    local h = test:height();
    local d = test:depth();
    local s = test:slices();
    local format = test:format();
    local flags = test:flags()

    if w ~= 884 then
        print("create image is " .. w .. " wide it should be 884")
    end
    if h ~= 406 then
        print("create image is " .. h .. " high it should be 406")
    end
    if d ~= 1 then
        print("create image is " .. d .. " depth it should be 1")
    end
    if s ~= 1 then
        print("create image is " .. s .. " slices it should be 1")
    end
    if format ~= "R8G8B8A8_UNORM" then
        print("create image is " .. format .. " and should be R8G8B8A8_UNORM")
    end
    if flags.Cubemap == true then
        print("create image marked as a Cubemaps and shouldn't be")
    end
    if flags.HeaderOnly == true then
        print("create image marked as a header only  and shouldn't be")
    end
    -- should be a completely black with 0 alpha image
    test:saveAsPNG("test_createsave.png")
end

do
    local test = image.createNoClear(444, 333, 1, 1, "R8G8B8A8_UNORM")
    if test == nil then
        print("unable to be create image")
    end
    local w = test:width();
    local h = test:height();
    local d = test:depth();
    local s = test:slices();
    local format = test:format();
    local flags = test:flags()

    if w ~= 444 then
        print("create image is " .. w .. " wide it should be 444")
    end
    if h ~= 333 then
        print("create image is " .. h .. " high it should be 333")
    end
    if d ~= 1 then
        print("create image is " .. d .. " depth it should be 1")
    end
    if s ~= 1 then
        print("create image is " .. s .. " slices it should be 1")
    end
    if format ~= "R8G8B8A8_UNORM" then
        print("create image is " .. format .. " and should be R8G8B8A8_UNORM")
    end
    if flags.Cubemap == true then
        print("create image marked as a Cubemaps and shouldn't be")
    end
    if flags.HeaderOnly == true then
        print("create image marked as a header only  and shouldn't be")
    end
    -- save could be zeroed or random
    test:saveAsPNG("test_createsave1.png")
end

do
    local test = image.createNoClear(256, 256, 1, 1, "R8G8B8A8_UNORM")
    if test == nil then
        print("unable to be create image")
    end

    for y = 1, 256 do
        for x = 1, 256 do
            local i = (y-1) * 256 + (x-1)
            test:setPixelAt(i, (x-1.0)/255.0, (y-1.0)/255.0, (x-1.0)/255.0, (y-1.0)/255.0)
        end
    end
    test:saveAsPNG("test_setpixel.png")

    local loadtest = image.load("test_setpixel.png")
    for y = 1, 256 do
        for x = 1, 256 do
            local i = (y-1) * 256 + (x-1)
            local r, g, b, a = loadtest:getPixelAt(i)
            if(r ~= (x-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. r .. " red incorrect in test_setpixel.png")
            end
            if(g ~= (y-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. g .. " green incorrect in test_setpixel.png")
            end
            if(b ~= (x-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. b .. " blue incorrect in test_setpixel.png")
            end
            if(a ~= (y-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. a .. " alpha incorrect in test_setpixel.png")
            end
        end
    end
end

do
    local loadtest = image.load("test_setpixel.png")
    for y = 1, 256 do
        for x = 1, 256 do
            local i = loadtest:calculateIndex(x-1,y-1, 0, 0)

            local r, g, b, a = loadtest:getPixelAt(i)
            if(r ~= (x-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. r .. " red incorrect in calculateIndex test")
            end
            if(g ~= (y-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. g .. " green incorrect in calculateIndex test")
            end
            if(b ~= (x-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. b .. " blue incorrect in calculateIndex test")
            end
            if(a ~= (y-1.0)/255.0) then
                print( x .. ", " .. y .. ": " .. a .. " alpha incorrect in calculateIndex test")
            end
        end
    end
end