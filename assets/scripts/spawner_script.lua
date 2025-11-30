-- spawner_script.lua
-- Example: Spawns new entities with components dynamically

local spawnTimer = 0
local spawnInterval = 3.0  -- Spawn every 3 seconds
local spawnedEntities = {}

function OnInit()
    Log("Spawner script initialized!")
    
    -- Make sure spawner entity has transform
    if not HasComponent(self, "Transform") then
        AddTransform(self, 0.0, 0.0)
    end
end

function OnUpdate(dt)
    spawnTimer = spawnTimer + dt
    
    -- Spawn a new entity every interval
    if spawnTimer >= spawnInterval then
        SpawnNewEntity()
        spawnTimer = 0
    end
    
    -- Update all spawned entities
    UpdateSpawnedEntities(dt)
end

function SpawnNewEntity()
    -- Create new entity
    local newEntity = CreateEntity()
    Log("Spawned new entity: " .. newEntity)
    
    -- Add components to it
    local offsetX = math.random(-1.0, 1.0)
    local offsetY = math.random(-0.5, 0.5)
    
    AddTransform(newEntity, offsetX, offsetY)
    AddSprite(newEntity, "quad")
    AddRenderable(newEntity)
    
    -- Store it in our list
    table.insert(spawnedEntities, {
        id = newEntity,
        lifetime = 0
    })
    
    Log("Created entity at (" .. offsetX .. ", " .. offsetY .. ")")
end

function UpdateSpawnedEntities(dt)
    -- Update each spawned entity
    for i = #spawnedEntities, 1, -1 do
        local data = spawnedEntities[i]
        data.lifetime = data.lifetime + dt
        
        -- Make them fall down
        local x, y = GetPosition(data.id)
        if x and y then
            SetPosition(data.id, x, y - dt * 0.2)
        end
        
        -- Destroy after 10 seconds
        if data.lifetime > 10.0 then
            if IsEntityValid(data.id) then
                DestroyEntity(data.id)
                Log("Destroyed entity " .. data.id .. " after lifetime expired")
            end
            table.remove(spawnedEntities, i)
        end
    end
end

function OnDestroy()
    -- Clean up all spawned entities
    Log("Cleaning up " .. #spawnedEntities .. " spawned entities")
    
    for _, data in ipairs(spawnedEntities) do
        if IsEntityValid(data.id) then
            DestroyEntity(data.id)
        end
    end
    
    spawnedEntities = {}
end
